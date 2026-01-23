import logging

logger = logging.getLogger(__name__)

import os
from pathlib import Path
import csv
from datetime import datetime

from flask import Blueprint
from flask_security import roles_required
from sqlalchemy import inspect

from utils import download_from_azure_blob

from app.db import DB
from app.battery import get_battery_info_entry, get_battery_data_table
from app.twin import add_to_prediction_features

data = Blueprint("data", __name__, url_prefix="/data")


def import_data(csv_path: str, esp_id: int):
    """
    Creates example data for fresh website lacking real data.
    """
    if not Path(csv_path).exists():
        return False

    name = f"battery_data_bms_{esp_id}"
    inspector = inspect(DB.engine)
    if esp_id == 999 and inspector.has_table(name):
        return False

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
                    "C": float(row["Capacity"] or 0),
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
                    "CC": int(row["Cycle"] or 0),
                    "wifi": False,
                }
            )

            add_to_prediction_features(esp_id, int(row["Cycle"]) - 1)

    if rows:
        try:
            DB.session.execute(table.insert(), rows)
            DB.session.commit()
        except Exception as e:
            logger.error(f"Error: {e}")
            return False

    return True


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
        RUNNING_IN_AZURE = "WEBSITE_INSTANCE_ID" in os.environ
        if RUNNING_IN_AZURE:
            path = "/tmp"
        else:
            path = "../simulation/data"
        logger.info(path)

        for dataset, esp_id in zip(
            ["normal", "low_power", "short_duration"], [996, 997, 998]
        ):
            i = 0
            import_success = True
            while import_success:
                if path == "/tmp":
                    Path(f"/tmp/{dataset}/").mkdir(exist_ok=True)
                    try:
                        download_from_azure_blob(
                            f"/tmp/{dataset}/data_{i+1}.csv",
                            "example-data",
                            f"simulated_data/{dataset}/data_{i+1}.csv",
                        )
                    except Exception as e:
                        logger.error(f"Could not download blob data: {e}")
                import_success = import_data(f"{path}/{dataset}/data_{i+1}.csv", esp_id)
                i += 1
                break
        return {}, 200
    except Exception as e:
        logger.error(e)
        return {}, 404
