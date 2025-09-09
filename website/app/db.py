import logging
logger = logging.getLogger(__name__)

from datetime import datetime, timedelta
from collections import defaultdict

from flask import Blueprint, request, jsonify
from flask_sqlalchemy import SQLAlchemy
from flask_security import roles_required, login_required
from sqlalchemy import inspect, insert, select, desc, text, Table

from utils import process_telemetry_data

db = Blueprint("db", __name__, url_prefix="/api/db")

DB = SQLAlchemy()
class BatteryInfo(DB.Model):
    """
        Defines the structure of the battery_info table, which manages the individual battery_data tables.
        Must first create the database manually in mysql command prompt:
            $ mysql -u root -p
            mysql> create database battery_data;
        Then initiate with flask:
            $ flask db init
            $ flask db migrate -m "initiate"
            $ flask db upgrade
        Further changes made to the class here are then propagated with:
            $ flask db migrate -m "add/remove ___ column"
            $ flask db upgrade
    """
    __tablename__ = "battery_info"

    esp_id = DB.Column(DB.String(7), primary_key=True)
    root_id = DB.Column(DB.String(7), nullable=True)
    last_updated_time = DB.Column(DB.DateTime, default=datetime.now, onupdate=datetime.now, nullable=False)
    live_websocket = DB.Column(DB.Boolean, default=False, nullable=False)


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
        
        # first update battery_info table
        try:
            # check if the entry already exists
            battery = DB.session.get(BatteryInfo, esp_id)
            if not battery:
                # create default one if not
                battery = BatteryInfo(
                    esp_id = "",
                    root_id = None,
                    last_updated_time = datetime.now(),
                    live_websocket = False,
                )
                DB.session.add(battery)
                logger.info(f"inserted new battery_info row with esp_id: {esp_id}")
            # update the entry info
            battery.esp_id = esp_id
            battery.root_id = None if i==0 else root_id
            battery.last_updated_time = datetime.now()
            DB.session.commit()
            set_live_websocket(esp_id, True) # this function (`update_battery_data`) is only ever called from the /esp_ws handler, so must be True
        except Exception as e:
            DB.session.rollback()
            logger.error(f"DB error processing WebSocket message from {esp_id}: {e}")

        # then update/create battery_data_<esp_id> table
        battery_data_table = get_battery_data_table(esp_id)
        try:
            process_telemetry_data(content) # first add approximate cell charges
            statement = insert(battery_data_table).values(
                t = datetime.now(), # TODO: update this to GPS data
                Q = content["Q"],
                H = content["H"],
                V = content["V"] / 10,
                V1 = content["V1"] / 10,
                V2 = content["V2"] / 10,
                V3 = content["V3"] / 10,
                V4 = content["V4"] / 10,
                I = content["I"] / 10,
                I1 = content["I1"] / 10,
                I2 = content["I2"] / 10,
                I3 = content["I3"] / 10,
                I4 = content["I4"] / 10,
                aT = content["aT"] / 10,
                cT = content["cT"] / 10,
                T1 = content["T1"] / 10,
                T2 = content["T2"] / 10,
                T3 = content["T3"] / 10,
                T4 = content["T4"] / 10,
                OTC = content["OTC"],
                wifi = content["wifi"],
            )
            DB.session.execute(statement)
            DB.session.commit()
        except Exception as e:
            DB.session.rollback()
            logger.error(f"DB error inserting data from {esp_id} into table: {e}")


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
        Returns a handle to a specified battery_data_<esp_id> table in the database.
        A new table is created if one does not yet exist.
    """
    name = f"battery_data_{esp_id}"

    # check if the table already exists
    inspector = inspect(DB.engine)
    if inspector.has_table(name):
        table = DB.Table(name, DB.metadata, autoload_with=DB.engine)
    
    # create one if not
    else:
        table = DB.Table(name,
            DB.Column("t", DB.DateTime, nullable=False, default=datetime.now, primary_key=True),
            DB.Column("Q", DB.Integer, nullable=False),
            DB.Column("H", DB.Integer, nullable=False),
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
            DB.Column("wifi", DB.Boolean, nullable=False),
        )
        table.create(bind=DB.engine, checkfirst=True)
        logger.info(f"created new table: {name}")

    return table


@db.route("/execute_sql", methods=["POST"])
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
            return jsonify({"confirm": "Please re-enter the query to confirm execution."}), 403
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


@db.route("/info", methods=["GET"])
@login_required
def info():
    """
        API used by frontend to fetch live_websocket statuses and mesh structure from battery_info table.
    """
    esp_dict = defaultdict()
    nodes_dict = defaultdict(list)
    batteries = DB.session.query(BatteryInfo).all()
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


@db.route("/data", methods=["GET"])
@login_required
def data():
    """
        API used by frontend to fetch most recent row in battery_data_<esp_id> table.
    """
    esp_id = request.args.get("esp_id")
    table_name = f"battery_data_{esp_id}"
    data_table = DB.Table(table_name, DB.metadata, autoload_with=DB.engine)

    statement = select(data_table).order_by(desc(data_table.c.t)).limit(1)

    with DB.engine.connect() as conn:
        row = conn.execute(statement).first()

    if row:
        return dict(row._mapping)
    else:
        return {}, 404


@db.route("/esp_ids", methods=["GET"])
# @login_required # log in not required otherwise can't be used in chart data (see below)
def esp_ids():
    """
        API used by frontend to fetch all esp_ids from battery_info table.
    """
    batteries = DB.session.query(BatteryInfo).all()
    return jsonify([battery.esp_id for battery in batteries])


@db.route("/chart_data", methods=["GET"])
# @login_required # log in not required otherwise wouldn't show in homepage
def chart_data():
    """
        API used by frontend to fetch 250 entries from battery_data_<esp_id> table for chart display.
    """
    esp_id = request.args.get("esp_id")
    if esp_id == "unavailable!":
        return {}, 404

    table_name = f"battery_data_{esp_id}"
    data_table = DB.Table(table_name, DB.metadata, autoload_with=DB.engine)
    column = request.args.get("column")

    sub_query = (select(data_table.c.t, data_table.c[column]).order_by(desc(data_table.c.t)).limit(250).subquery()) # get the 250 most recent (time-wise) entries
    statement = select(sub_query)

    with DB.engine.connect() as conn:
        rows = conn.execute(statement)

    if rows:
        rows = rows.fetchall()

        previous = None
        data = []
        for row in rows:

            # take only data from most recent date
            if row[0].date() != rows[-1][0].date():
                continue

            if previous is not None:
                if previous - row[0] > timedelta(minutes = 5):
                    break
            previous = row[0]

            data.append({"timestamp": row[0], column: row[1]})

        # reorder chronologically
        data = data[::-1]
        return jsonify(data)
    else:
        return {}, 404
