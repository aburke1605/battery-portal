import logging

logger = logging.getLogger(__name__)

from datetime import datetime, timedelta
from collections import defaultdict

from flask import Blueprint, request, jsonify
from flask_security import roles_required, login_required
from flask_login import current_user
from sqlalchemy import inspect, insert, select, desc, func, text, Table

from utils import process_telemetry_data

from app.db import DB, BatteryInfo, PredictionFeatures

battery = Blueprint("battery", __name__, url_prefix="/battery")


def get_battery_info_entry(esp_id: str) -> BatteryInfo:
    # check if the entry already exists
    battery = DB.session.get(BatteryInfo, esp_id)

    if not battery:
        # create default one if not
        battery = BatteryInfo(
            esp_id=0,
            root_id=None,
            last_updated_time=datetime.now(),
            live_websocket=False,
        )
        DB.session.add(battery)
        logger.info(f"inserted new battery_info row with esp_id: {esp_id}")

    return battery


def set_live_websocket(esp_id: str, live: bool) -> None:
    """
    Simple utility function to change the live_websocket value for a given battery unit in the battery_info table.
    """
    try:
        battery = DB.session.get(BatteryInfo, esp_id)
        battery.live_websocket = live
        DB.session.commit()
    except Exception as e:
        DB.session.rollback()
        logger.error(f"DB error resetting WebSocket status for {esp_id}: {e}")


def get_battery_data_table(esp_id: str) -> Table:
    """
    Returns a handle to a specified battery_data_bms_<esp_id> table in the database.
    A new table is created if one does not yet exist.
    """
    name = f"battery_data_bms_{esp_id}"

    # check if the table already exists
    inspector = inspect(DB.engine)
    if inspector.has_table(name):
        table = DB.Table(name, DB.metadata, autoload_with=DB.engine)

    # create one if not
    else:
        table = DB.Table(
            name,
            DB.Column(
                "timestamp",
                DB.DateTime,
                nullable=False,
                default=datetime.now,
                primary_key=True,
            ),
            DB.Column("lat", DB.Float, nullable=False),
            DB.Column("lon", DB.Float, nullable=False),
            DB.Column("Q", DB.Integer, nullable=False),
            DB.Column("H", DB.Integer, nullable=False),
            DB.Column("C", DB.Float, nullable=False),
            DB.Column("V", DB.Float, nullable=False),
            DB.Column("V1", DB.Float, nullable=False),
            DB.Column("V2", DB.Float, nullable=False),
            DB.Column("V3", DB.Float, nullable=False),
            DB.Column("V4", DB.Float, nullable=False),
            DB.Column("I", DB.Float, nullable=False),
            DB.Column("I1", DB.Float, nullable=False),
            DB.Column("I2", DB.Float, nullable=False),
            DB.Column("I3", DB.Float, nullable=False),
            DB.Column("I4", DB.Float, nullable=False),
            DB.Column("aT", DB.Float, nullable=False),
            DB.Column("cT", DB.Float, nullable=False),
            DB.Column("T1", DB.Float, nullable=False),
            DB.Column("T2", DB.Float, nullable=False),
            DB.Column("T3", DB.Float, nullable=False),
            DB.Column("T4", DB.Float, nullable=False),
            DB.Column("OTC", DB.Integer, nullable=False),
            DB.Column("CC", DB.Integer, nullable=False),
            DB.Column("wifi", DB.Boolean, nullable=False),
        )
        table.create(bind=DB.engine, checkfirst=True)
        logger.info(f"created new table: {name}")

    return table


from app.twin import add_to_prediction_features


def update_battery_data(json: list) -> None:
    """
    Is called when new telemetry data is received from an ESP32 WebSocket client.
    Creates new / updates the battery_data_<esp_id> table and corresponding row in the battery_info table.
    """
    # first grab the esp_id of the root for later use
    root_id = json[0]["esp_id"]

    for i, data in enumerate(json):
        esp_id = data["esp_id"]
        content = data["content"]

        if content["I"] == 0:
            continue

        # first update battery_info table
        try:
            # check if the entry already exists
            battery = get_battery_info_entry(esp_id)
            # update the entry info
            battery.esp_id = esp_id
            battery.root_id = None if i == 0 else root_id
            DB.session.commit()
            set_live_websocket(
                esp_id, True
            )  # this function (`update_battery_data`) is only ever called from the /esp_ws handler, so must be True
        except Exception as e:
            DB.session.rollback()
            logger.error(f"DB error processing WebSocket message from {esp_id}: {e}")

        # then update/create battery_data_bms_<esp_id> table
        battery_data_table = get_battery_data_table(esp_id)
        try:
            process_telemetry_data(content)  # first add approximate cell charges
            query = insert(battery_data_table).values(
                timestamp=datetime.fromisoformat(content["timestamp"]),
                lat=content["lat"],
                lon=content["lon"],
                Q=content["Q"],
                H=content["H"],
                C=content["C"],
                V=content["V"] / 10,
                V1=content["V1"] / 100,
                V2=content["V2"] / 100,
                V3=content["V3"] / 100,
                V4=content["V4"] / 100,
                I=content["I"] / 10,
                I1=content["I1"] / 100,
                I2=content["I2"] / 100,
                I3=content["I3"] / 100,
                I4=content["I4"] / 100,
                aT=content["aT"] / 10,
                cT=content["cT"] / 10,
                T1=content["T1"] / 100,
                T2=content["T2"] / 100,
                T3=content["T3"] / 100,
                T4=content["T4"] / 100,
                OTC=content["OTC"],
                CC=content["CC"],
                wifi=content["wifi"],
            )
            DB.session.execute(query)
            DB.session.commit()
        except Exception as e:
            DB.session.rollback()
            logger.error(f"DB error inserting data from {esp_id} into table: {e}")

        # finally, process previous cycle blocks into features for ML model
        cycle_index = int(content["CC"]) - 1
        if (
            cycle_index >= 200
        ):  # largest block feature is 200 cycles, so skip until then
            if (cycle_index) % 10 == 0:  # make a new datapoint every 10 cycles
                if (
                    PredictionFeatures.query.filter_by(
                        esp_id=esp_id, cycle_index=cycle_index
                    ).first()
                    is None
                ):  # only if it doesn't already exist
                    add_to_prediction_features(esp_id, cycle_index)


@battery.route("/execute_sql", methods=["POST"])
@roles_required("superuser")
def execute_sql():
    """
    API used by frontend to enable manual sql execution on database.
    Requires login as admin.
    """
    data = request.get_json()
    query = data.get("query")

    if not query:
        return jsonify({"error": "No query provided"}), 400

    verified = True if query[:6] == "ADMIN " else False
    dangerous_statements = ["drop", "delete", "update", "alter", "truncate"]
    if any(word in query.lower() for word in dangerous_statements):
        if not verified:
            return (
                jsonify({"confirm": "Please re-enter the query to confirm execution."}),
                403,
            )
        else:
            query = query[5:]

    result, status = execute_query(query)
    return jsonify(result), status


def execute_query(query: str):
    try:
        result = DB.session.execute(text(query))
        if result.returns_rows:
            # SELECT or anything returning rows
            rows = [dict(row._mapping) for row in result]
            return {"rows": rows}, 200
        else:
            # it's DML (INSERT/UPDATE/DELETE)
            DB.session.commit()
            return {"rows_affected": result.rowcount}, 200

    except Exception as e:
        DB.session.rollback()
        return {"error": str(e)}, 400


@battery.route("/info", methods=["GET"])
@login_required
def info():
    """
    API used by frontend to fetch live_websocket statuses and mesh structure from battery_info table.
    """
    esp_dict = defaultdict()
    nodes_dict = defaultdict(list)
    DB.session.remove()
    if any(role.name == "superuser" for role in current_user.roles):
        batteries = DB.session.query(BatteryInfo)
    else:
        batteries = DB.session.query(BatteryInfo).where(
            BatteryInfo.owner_id == current_user.id
        )
    for battery in batteries:
        esp_dict[battery.esp_id] = {
            "esp_id": battery.esp_id,
            "root_id": battery.root_id,
            "last_updated_time": battery.last_updated_time,
            "live_websocket": battery.live_websocket,
            "nodes": [],
        }
        if battery.root_id:
            nodes_dict[battery.root_id].append(battery.esp_id)

    # add nodes to each root
    for root_id, node_ids in nodes_dict.items():
        if root_id in esp_dict:
            for node_id in node_ids:
                esp_dict[root_id]["nodes"].append(esp_dict[node_id])

    # return only roots
    return jsonify([esp for esp in esp_dict.values() if esp["root_id"] is None])


@battery.route("/data", methods=["GET"])
@login_required
def data():
    """
    API used by frontend to fetch most recent row in battery_data_bms_<esp_id> table.
    """
    esp_id = request.args.get("esp_id")
    table_name = f"battery_data_bms_{esp_id}"
    data_table = DB.Table(table_name, DB.metadata, autoload_with=DB.engine)

    # this really is ordered by most recent and not just date or time individually
    # fmt: off
    query = (
        select(data_table)
        .order_by(desc(data_table.c.timestamp))
        .limit(1)
    )
    # fmt: on

    row = DB.session.execute(query).first()

    if row:
        return dict(row._mapping)
    else:
        return {}, 404


@battery.route("/esp_ids", methods=["GET"])
# @login_required # log in not required otherwise can't be used in chart data (see below)
def esp_ids():
    """
    API used by frontend to fetch all esp_ids from battery_info table.
    """
    batteries = DB.session.query(BatteryInfo).all()
    return jsonify([battery.esp_id for battery in batteries])


@battery.route("/chart_data", methods=["GET"])
# @login_required # log in not required otherwise wouldn't show in homepage
def chart_data():
    """
    API used by frontend to fetch 250 entries from battery_data_bms_<esp_id> table for chart display.
    """
    esp_id = request.args.get("esp_id")
    if esp_id == "unavailable!" or esp_id == "undefined" or esp_id == "":
        return {}, 200

    table_name = f"battery_data_bms_{esp_id}"
    data_table = DB.Table(table_name, DB.metadata, autoload_with=DB.engine)
    column = request.args.get("column")

    # fmt: off
    sub_sub_query = (
        select(
            data_table.c.timestamp,
            data_table.c[column],  # query timestamp and column of interest
            func.row_number()
            .over(
                order_by=desc(data_table.c.timestamp)  # ordered by most recent (time-wise)
            )
            .label("rn")
        )
        .subquery()
    )
    sub_query = (
        select(
            sub_sub_query.c.timestamp,
            sub_sub_query.c[column]
        )
        .where(sub_sub_query.c.rn % 1 == 0)  # every 1th row so query is not too large
        .limit(250)  # max 250 data points
        .subquery()
    )
    query = (
        select(sub_query)
    )
    # fmt: on

    rows = DB.session.execute(query).fetchall()

    if rows:
        previous = None
        data = []
        for row in rows:
            # take only data from most recent date
            if row[0].date() != rows[-1][0].date():
                continue
            if previous is not None:
                if previous - row[0] > timedelta(days=1):
                    break
            previous = row[0]

            data.append({"timestamp": row[0], column: row[1]})

        # reorder chronologically
        data = data[::-1]
        return jsonify(data)
    else:
        return {}, 404
