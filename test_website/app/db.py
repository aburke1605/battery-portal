from flask import Blueprint, request, jsonify
from flask_sqlalchemy import SQLAlchemy
from sqlalchemy import Table, Column, inspect, insert, select, desc, DateTime, Integer

from datetime import datetime
from collections import defaultdict

db = Blueprint("db", __name__, url_prefix="/api/db")

DB = SQLAlchemy()
class battery_info(DB.Model):
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
            battery = DB.session.get(battery_info, esp_id)
            if not battery:
                # create default one if not
                battery = battery_info(
                    esp_id = "",
                    root_id = None,
                    last_updated_time = datetime.now(),
                    live_websocket = False,
                )
                DB.session.add(battery)
                print(f"inserted new battery_info row with esp_id: {esp_id}")
            # update the entry info
            battery.esp_id = esp_id
            battery.root_id = None if i==0 else root_id
            battery.last_updated_time = datetime.now()
            DB.session.commit()
            set_live_websocket(esp_id, True) # this function (`update_battery_data`) is only ever called from the /esp_ws handler, so must be True
        except Exception as e:
            DB.session.rollback()
            print(f"DB error processing WebSocket message from {esp_id}: {e}")

        # then update/create battery_data_<esp_id> table
        battery_data_table = get_battery_data_table(esp_id)
        try:
            statement = insert(battery_data_table).values(
                t = datetime.now(), # TODO: update this to GPS data
                Q = content["Q"],
                H = content["H"],
            )
            DB.session.execute(statement)
            DB.session.commit()
        except Exception as e:
            DB.session.rollback()
            print(f"DB error inserting data from {esp_id} into table: {e}")


def set_live_websocket(esp_id: str, live: bool) -> None:
    """
        Simple utility function to change the live_websocket value for a given battery unit in the battery_info table.
    """
    try:
        battery = DB.session.get(battery_info, esp_id)
        battery.live_websocket = live
        DB.session.commit()
    except Exception as e:
        DB.session.rollback()
        print(f"DB error resetting WebSocket status for {esp_id}: {e}")


def get_battery_data_table(esp_id: str) -> Table:
    """
        Returns a handle to a specified battery_data_<esp_id> table in the database.
        A new table is created if one does not yet exist.
    """
    name = f"battery_data_{esp_id}"

    # check if the table already exists
    inspector = inspect(DB.engine)
    if inspector.has_table(name):
        table = Table(name, DB.metadata, autoload_with=DB.engine)
    
    # create one if not
    else:
        table = Table(name, DB.metadata,
            Column("t", DateTime, nullable=False, default=datetime.now, primary_key=True),
            Column("Q", Integer, nullable=False),
            Column("H", Integer, nullable=False),
        )
        table.create(bind=DB.engine)
        print(f"created new table: {name}")

    return table


@db.route("/")
def index():
    return "<p>Hello from DB!</p>"


@db.route("/info")
def info():
    """
        API used by frontend to fetch live_websocket statuses and mesh structure from battery_info table.
    """
    esp_dict = defaultdict()
    nodes_dict = defaultdict(list)
    batteries = DB.session.query(battery_info).all()
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


@db.route("/data")
def data():
    """
        API used by frontend to fetch most recent row in battery_data_<esp_id> table.
    """
    esp_id = request.args.get("esp_id")
    table_name = f"battery_data_{esp_id}"
    data_table = Table(table_name, DB.metadata, autoload_with=DB.engine)

    statement = select(data_table).order_by(desc(data_table.c.t)).limit(1)

    with DB.engine.connect() as conn:
        row = conn.execute(statement).first()

    if row:
        return dict(row._mapping)
    else:
        return {}, 404
