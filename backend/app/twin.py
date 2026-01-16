import logging

logger = logging.getLogger(__name__)

from datetime import timedelta
import numpy as np

from flask import Blueprint, request
from flask_security import roles_required, login_required
from sqlalchemy import select, desc, asc

twin = Blueprint("twin", __name__, url_prefix="/twin")

from app.battery import DB, import_data, get_query_size


@twin.route("/simulation", methods=["GET"])
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


def high_temperature_check(esp_id: str, high_temperature_threshold: float) -> bool:
    try:
        table_name = f"battery_data_bms_{esp_id}"
        data_table = DB.Table(table_name, DB.metadata, autoload_with=DB.engine)

        query_size = get_query_size(data_table, 12)
        # fmt: off
        sub_query = (
            select(
                data_table.c.cT,
                data_table.c.timestamp
            )
            .order_by(desc(data_table.c.timestamp)) # order by most recent
            .limit(query_size)                      # limit to determined size
            .subquery()
        )
        query = (
            select(
                sub_query.c.cT,
            )
            .order_by(asc(sub_query.c.timestamp)) # reorder
        )
        # fmt: on
        data = np.array(DB.session.execute(query).scalars().all())

        return np.mean(data) > high_temperature_threshold

    except Exception as e:
        logger.error(f"Error: {e}")

    return False


def low_power_check(esp_id: str, power_threshold: float) -> bool:
    """
    Measures the average power in the last 12h worth of discharge data to check for low power usage.
    """

    try:
        table_name = f"battery_data_bms_{esp_id}"
        data_table = DB.Table(table_name, DB.metadata, autoload_with=DB.engine)

        # step 1: get query sie corresponding to 12h worth of data
        query_size = get_query_size(data_table, 12)

        # step 2: query current and voltage data for the 12h period
        # fmt: off
        query = (
            select(
                data_table.c.I,
                data_table.c.V
            )
            .order_by(desc(data_table.c.timestamp)) # order by most recent
            .where(data_table.c.I < 0)              # discharge only
            .limit(query_size)                      # limit to determined size
        )
        # fmt: on
        data = np.array(DB.session.execute(query).fetchall())

        # step 3: compare average power with threshold
        average_power = np.mean(-data[:, 0] * data[:, 1])
        return average_power < power_threshold
        # TODO:
        #     can we instead return the recommended SoC limits??

    except Exception as e:
        logger.error(f"Error: {e}")

    return False


def low_depth_of_discharge_check(
    esp_id: str, depth_of_discharge_threshold: float
) -> bool:
    min_downtime = timedelta(minutes=5)
    depths_of_discharge = []

    try:
        table_name = f"battery_data_bms_{esp_id}"
        data_table = DB.Table(table_name, DB.metadata, autoload_with=DB.engine)

        query_size = get_query_size(data_table, 12)
        # fmt: off
        sub_query = (
            select(
                data_table.c.I,
                data_table.c.V,
                data_table.c.timestamp
            )
            .order_by(desc(data_table.c.timestamp)) # order by most recent
            .limit(query_size)                      # limit to determined size
            .subquery()
        )
        query = (
            select(
                sub_query.c.I,
                sub_query.c.V,
                sub_query.c.timestamp
            )
            .order_by(asc(sub_query.c.timestamp)) # reorder
        )
        # fmt: on
        data = np.array(DB.session.execute(query).fetchall())

        clean_start = False
        start = None
        stop = None
        for i, (current, *_) in enumerate(data):
            # the below if clauses are in reverse logical order

            if start != None:
                # step 3: append
                if stop != None:
                    # check for breaks within a cycle
                    if not any(
                        data[j + 1][2] - data[j][2] > min_downtime
                        for j in range(start, stop - 1)
                    ):
                        depths_of_discharge.append(data[start][1] - data[stop][1])
                    start = None
                    stop = None
                    continue

                # step 2: find the stop point only after we've found the start point
                else:
                    if current > 0:
                        stop = i - 1

            # step 1: find the start point
            else:
                if current > 0:
                    # if the data begins in a discharge cycle, skip that segment
                    clean_start = True
                else:
                    if clean_start:
                        start = i

        if (
            len(depths_of_discharge) > 4
            and np.mean(depths_of_discharge) < depth_of_discharge_threshold
        ):
            return True

    except Exception as e:
        logger.error(f"Error: {e}")

    return False


@twin.route("/recommendation", methods=["GET"])
@login_required
def recommendation():
    """
    API used by frontend to fetch generated BMS optimisation recommendations based on recent telemetry.
    """
    try:
        esp_id = request.args.get("esp_id")
        recommendations = []

        high_temperature_threshold = 40  # °C
        if high_temperature_check(esp_id, high_temperature_threshold):
            I_max = 2.0
            recommendations.append(
                {
                    "type": "current-dischg-limit",
                    "message": f"Overheating detected (> {high_temperature_threshold:.1f}°C). Throttle discharge current to {I_max}A.",
                    "max": I_max,
                }
            )

        power_threshold = 2  # W
        if low_power_check(esp_id, power_threshold):
            recommendations.append(
                {
                    "type": "soc-window",
                    "message": f"Low average power usage (< {power_threshold}W) over last 12 hours of usage. Restrict SoC range to [{40},{60}]%.",
                    "min": 40,
                    "max": 60,
                }
            )

        depth_of_discharge_threshold = 1  # V
        if low_depth_of_discharge_check(esp_id, depth_of_discharge_threshold):
            recommendations.append(
                {
                    "type": "operating-voltage",
                    "message": f"Frequently low depth of discharge (ΔV < {depth_of_discharge_threshold}V) over last 12 hours of usage. Reduce operating voltage to {2.7 + depth_of_discharge_threshold}V.",
                    "max": 2.7 + depth_of_discharge_threshold,  # minimum + threshold
                }
            )

        return {"recommendations": recommendations}, 200

    except Exception as e:
        return {"Error": e}, 404
