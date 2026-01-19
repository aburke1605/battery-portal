import logging

logger = logging.getLogger(__name__)

import csv
from datetime import datetime

from flask import Blueprint
from flask_security import roles_required
from sqlalchemy import inspect

from app.db import DB
from app.battery import get_battery_info_entry, get_battery_data_table

data = Blueprint("data", __name__, url_prefix="/data")


def import_data(csv_path: str, esp_id: int):
    """
    Creates example data for fresh website lacking real data.
    """
    name = f"battery_data_bms_{esp_id}"
    inspector = inspect(DB.engine)
    if esp_id == 999 and inspector.has_table(name):
        return

    # create entry in battery_info table
    try:
        battery = get_battery_info_entry(esp_id)
        battery.esp_id = esp_id
        DB.session.commit()
    except Exception as e:
        DB.session.rollback()
        logger.error(f"DB error processing data import for {esp_id}: {e}")

    # create new battery_data_bms_<esp_id> table
    table = get_battery_data_table(str(esp_id))
    with open(csv_path) as f:
        # skip junk lines until header
        for line in f:
            if line.strip().startswith("TimeStamp"):
                header = line.strip().split(",")
                break
        reader = csv.DictReader(f, fieldnames=header)
        rows = []
        for row in reader:
            if not row["TimeStamp"].strip():
                continue
            if float(row["Current"]) == 0:
                continue
            rows.append(
                {
                    "timestamp": datetime.fromisoformat(row["TimeStamp"]),
                    "lat": 0,
                    "lon": 0,
                    "Q": int(row["Relative State of Charge"] or 0),
                    "H": int(row["State of Health"] or 0),
                    "V": float(row["Voltage"] or 0) / 1000,
                    "V1": 0,
                    "V2": 0,
                    "V3": 0,
                    "V4": 0,
                    "I": float(row["Current"] or 0) / 1000,
                    "I1": 0,
                    "I2": 0,
                    "I3": 0,
                    "I4": 0,
                    "aT": 0,
                    "cT": float(row["Temperature"] or 0),
                    "T1": 0,
                    "T2": 0,
                    "T3": 0,
                    "T4": 0,
                    "OTC": 0,
                    "wifi": False,
                }
            )
    if rows:
        DB.session.execute(table.insert(), rows)
        DB.session.commit()


@data.route("/example", methods=["GET"])
@roles_required("superuser")
def example():
    """
    API
    """
    try:
        import_data("GPCCHEM.csv", 999)
        return {}, 200
    except:
        return {}, 404


@data.route("/simulation", methods=["GET"])
@roles_required("superuser")
def simulation():
    """
    API
    """
    try:
        for i in range(900):
            import_data(f"../simulation/data/normal/data_{i+1}.csv", 996)
            import_data(f"../simulation/data/low_power/data_{i+1}.csv", 997)
            import_data(f"../simulation/data/short_duration/data_{i+1}.csv", 998)
        return {}, 200
    except:
        return {}, 404
