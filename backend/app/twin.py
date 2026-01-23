import logging

logger = logging.getLogger(__name__)

from datetime import datetime, timedelta
import numpy as np
from sklearn.gaussian_process import GaussianProcessRegressor
from sklearn.gaussian_process.kernels import RBF
import pandas as pd
from xgboost import XGBClassifier
from sklearn.model_selection import train_test_split
from sklearn.metrics import roc_curve

from flask import Blueprint, request
from flask_security import login_required
from sqlalchemy import select, desc, asc, Table

from utils import upload_to_azure_blob

from app.db import DB, PredictionFeatures
from app.battery import get_battery_data_table

twin = Blueprint("twin", __name__, url_prefix="/twin")


@twin.route("/TEST")
def TEST():
    train_model()
    return {}, 200


def add_to_prediction_features(esp_id: int, current_cycle: int) -> None:
    if current_cycle < 200:
        return  # largest block feature is 200 cycles, so skip until then
    if current_cycle % 10 != 0:
        return  # make a new datapoint every 10 cycles
    if (
        PredictionFeatures.query.filter_by(
            esp_id=esp_id, cycle_index=current_cycle
        ).first()
        is not None
    ):
        return  # only if it doesn't already exist

    try:
        table = get_battery_data_table(str(esp_id))
        cycles = np.arange(current_cycle + 1 - 50, current_cycle + 1, step=1)
        # fmt: off
        query = (
            select(
                table.c.CC,
                table.c.cT,
                table.c.Q,
                table.c.C,
            )
            .where(
                table.c.CC >= int(cycles[0]),
                table.c.CC <= int(cycles[-1]),
            )
        )
        # fmt: on
        block = np.array(DB.session.execute(query).fetchall())

        mean_temp_last_50_cycles = np.mean(block[:, 1])

        DoDs = []
        for cycle in cycles:
            mask = block[:, 0] == cycle
            if np.sum(mask) == 0:
                continue
            DoDs.append(min(block[mask][:, 2]))
        mean_DoD_last_50_cycles = np.mean(DoDs)

        capacity_Ah_last_50_cycles = np.mean(block[:, 3])

        # fmt: off
        query = (
            select(
                table.c.timestamp,
                table.c.C,
            )
            .where(
                table.c.CC == current_cycle - 200 + 1
            )
            .limit(1)
        )
        # fmt: on
        first = np.array(DB.session.execute(query).fetchall())
        # fmt: off
        query = (
            select(
                table.c.timestamp,
                table.c.C,
            )
            .where(
                table.c.CC == current_cycle
            )
            .limit(1)
        )
        # fmt: on
        second = np.array(DB.session.execute(query).fetchall())
        capacity_slope_last_200_cycles = (second[0][1] - first[0][1]) / (
            (second[0][0] - first[0][0]).total_seconds() / (60 * 60)
        )

        most_recent_timestamp = DB.session.execute(
            select(table.c.timestamp).order_by(desc(table.c.timestamp))
        ).first()[0]
        # fmt: off
        query = (
            select(
                table.c.timestamp,
                table.c.Q,
            )
            .where(
                table.c.timestamp > most_recent_timestamp - timedelta(days=7)
            )
        )
        # fmt: on
        block = np.array(DB.session.execute(query).fetchall())

        idle_hours_last_7d = 0
        min_downtime = timedelta(minutes=5)
        last_timestamp = block[0, 0]

        hours_soc_gt_90_last_7d = 0
        first_timestamp_above_90 = block[0, 0]
        Q_was_above_90 = False

        for timestamp, Q in block:
            if timestamp - last_timestamp > min_downtime:
                idle_hours_last_7d += (timestamp - last_timestamp).total_seconds()
            last_timestamp = timestamp

            if Q <= 90:
                if Q_was_above_90:
                    hours_soc_gt_90_last_7d += (
                        timestamp - first_timestamp_above_90
                    ).total_seconds()
                    Q_was_above_90 = False
            else:
                if not Q_was_above_90:
                    first_timestamp_above_90 = timestamp
                Q_was_above_90 = True

        idle_hours_last_7d /= 60 * 60
        hours_soc_gt_90_last_7d /= 60 * 60

        DB.session.add(
            PredictionFeatures(
                timestamp=second[0][0],
                esp_id=esp_id,
                cycle_index=current_cycle,
                mean_temp_last_50_cycles=mean_temp_last_50_cycles,
                mean_DoD_last_50_cycles=mean_DoD_last_50_cycles,
                capacity_Ah_last_50_cycles=capacity_Ah_last_50_cycles,
                capacity_slope_last_200_cycles=capacity_slope_last_200_cycles,
                hours_soc_gt_90_last_7d=hours_soc_gt_90_last_7d,
                idle_hours_last_7d=idle_hours_last_7d,
            )
        )
        DB.session.commit()
    except Exception:
        DB.session.rollback()
        logger.exception("Error committing prediction features to database")


def decide_failure_within_7d(esp_id: int) -> None:
    try:
        table = get_battery_data_table(str(esp_id))

        # get first timestamp where H is 80

        # fmt: off
        query = (
            select(table.c.timestamp)
            .where(table.c.H < 81)
            .limit(1)
        )
        # fmt: on
        timestamp = DB.session.execute(query).scalars().all()[0]

        for row in PredictionFeatures.query.filter(
            PredictionFeatures.esp_id == esp_id,
            PredictionFeatures.timestamp > timestamp - timedelta(days=7),
        ).all():
            row.failure_within_7d = True
        DB.session.commit()

    except Exception as e:
        DB.session.rollback()
        logger.error(f"Error updating failure within 14 days boolean: {e}")


def train_model():
    esp_ids = DB.session.query(PredictionFeatures.esp_id).distinct().all()

    for esp_id in [row[0] for row in esp_ids]:
        decide_failure_within_7d(esp_id)

    dataframe = pd.read_sql(select(PredictionFeatures), DB.engine)
    y = dataframe["failure_within_7d"].to_numpy()
    X = dataframe.drop(
        ["timestamp", "esp_id", "failure_within_7d"], axis="columns"
    ).to_numpy()

    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.25)
    bst = XGBClassifier(
        n_estimators=2, max_depth=2, learning_rate=1, objective="binary:logistic"
    )
    bst.fit(X_train, y_train)
    pred_proba = bst.predict_proba(X_test)

    fpr, tpr, *_ = roc_curve(y_test, pred_proba[:, 1])
    import matplotlib.pyplot as plt

    fig, ax = plt.subplots()
    ax.plot([0, 1], [0, 1], linestyle=":")
    ax.scatter(fpr, tpr)
    fig.savefig("ROC.pdf")

    # =====================================
    # =====================================
    local_path = "/tmp/xgb_v1.json"
    bst.save_model(local_path)
    upload_to_azure_blob(local_path, "xgb-models", "xgb/v1/model.json")
    # =====================================
    # =====================================


def get_query_size(data_table: Table, hours: float) -> int:
    """
    Since data is recorded only when the current is not zero, the data may be 'chunked', with lengthy periods of downtime separating each.
    The 12h therefore must be acquired cumulatively, where we define some minimum downtime period (e.g. 5m) which separates consecutive chunks.

    TODO:
        Assuming any connected battery unit sends data on average every one minute, it would be simpler to just do a `.limit(hours*60)`.
        So, check if the assumption is true!
    """

    target_duration = timedelta(hours=hours)
    min_downtime = timedelta(minutes=5)
    batch_size = 60

    last_timestamp = datetime.now()
    cumulative_duration = timedelta(0)

    query_size = 1  # at least

    try:
        # progressively fetch timestamps until get >=hours of collected time
        while cumulative_duration < target_duration:
            # initial query
            # fmt: off
            sub_query = (
                select(data_table.c.timestamp)
                .order_by(desc(data_table.c.timestamp))          # order by most recent
                .where(data_table.c.timestamp <= last_timestamp) # skip previously queried batches
                .limit(batch_size)                               # query in batches to avoid large queries
                .subquery()
            )
            query = (
                select(sub_query.c.timestamp)
                .order_by(asc(sub_query.c.timestamp)) # reorder
            )
            # fmt: on
            timestamps = DB.session.execute(query).scalars().all()
            if not timestamps:
                break

            # measure accurate cumulative time by splitting into chunks
            chunks = []  # will be 2D array; list of lists of timestamps
            current_chunk = [timestamps[0]]  # first chunk, starts from the beginning
            for previous_timestamp, next_timestamp in zip(timestamps, timestamps[1:]):
                if (next_timestamp - previous_timestamp) < min_downtime:
                    current_chunk.append(next_timestamp)
                else:
                    chunks.append(current_chunk)  # chunk complete
                    current_chunk = [next_timestamp]  # start new chunk for next loop
            chunks.append(current_chunk)  # final chunk

            reversed_chunks = reversed(chunks)

            # increment cumulative time by chunk time periods and number of data points
            for chunk in reversed_chunks:
                cumulative_duration += chunk[-1] - chunk[0]
                query_size += len(chunk)
                if cumulative_duration >= target_duration:
                    break  # break early, no need to continue to next chunk or batch

            # prepare for next batch loop
            last_timestamp = timestamps[0]
            query_size -= 1  # because first time in batch i == last time in batch i-1
            # (see `where(data_table.c.timestamp <= last_timestamp)` in inital query)
            if len(timestamps) < batch_size:
                break  # no more data in table

    except Exception as e:
        logger.error(f"Error: {e}")

    return query_size


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

        current_threshold = 0.01
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
                    if current > current_threshold:
                        stop = i - 1

            # step 1: find the start point
            else:
                if current > current_threshold:
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


@twin.route("/test", methods=["GET"])
def test():
    predict_soh(request.args.get("esp_id"))
    return {}, 200


def extract_features(discharge_cycles, charge_cycles):
    TIECVD = []
    TIEDVD = []
    for discharge_cycle, charge_cycle in zip(discharge_cycles, charge_cycles):
        if len(discharge_cycle) > 1:
            dv = np.diff(discharge_cycle[:, 1])
            dt = np.diff(discharge_cycle[:, 2])
            dt = np.array([x.total_seconds() for x in dt])
            TIECVD.append(np.sum(np.abs(dv) * dt))
        else:
            TIECVD.append(np.nan)

        if len(charge_cycle) > 1:
            dv = np.diff(charge_cycle[:, 1])
            dt = np.diff(charge_cycle[:, 2])
            dt = np.array([x.total_seconds() for x in dt])
            TIEDVD.append(np.sum(np.abs(dv) * dt))
        else:
            TIEDVD.append(np.nan)

    return np.array(TIECVD), np.array(TIEDVD)


def predict_soh(esp_id: str) -> None:
    table_name = f"battery_data_bms_{esp_id}"
    data_table = DB.Table(table_name, DB.metadata, autoload_with=DB.engine)

    cycles = {}

    # fmt: off
    sub_query = (
        select(
            data_table.c.I,
            data_table.c.V,
            data_table.c.CC,
            data_table.c.timestamp
        )
        .subquery()
    )
    # fmt: on
    N_cycles = 91
    for cycle_count in range(1, N_cycles + 1):
        cycles[cycle_count] = {"charge": [], "discharge": []}
        for cycle_type, negative_current in zip(["charge", "discharge"], [False, True]):
            # fmt: off
            query = (
                select(
                    sub_query.c.I,
                    sub_query.c.V,
                    sub_query.c.timestamp
                )
                .where(sub_query.c.CC == cycle_count)
                .where((sub_query.c.I < 0) == negative_current)
                .limit(10)                                      # remove me
            )
            # fmt: on
            data = DB.session.execute(query).fetchall()
            if len(data) > 0:
                cycles[cycle_count][cycle_type] = data

    # remove cycles with missing charge or discharge data
    new_N_cycles = N_cycles
    for cycle_count in range(1, N_cycles + 1):
        if (
            len(cycles[cycle_count]["charge"]) == 0
            or len(cycles[cycle_count]["discharge"]) == 0
        ):
            cycles.pop(cycle_count)
            new_N_cycles -= 1
    N_cycles = new_N_cycles

    charge_cycles = []
    discharge_cycles = []
    capacity_per_cycle = []

    for cycle in sorted(cycles.keys()):
        charge_cycle = np.array(cycles[cycle]["charge"])
        discharge_cycle = np.array(cycles[cycle]["discharge"])

        if len(discharge_cycle) > 1:
            dt = np.diff(discharge_cycle[:, 2])
            dt = np.array([x.total_seconds() for x in dt])
            I = discharge_cycle[:-1, 0].astype(float)
            capacity = -np.sum(I * dt) / (60**2)  # convert to Amp-hours
        else:
            capacity = np.nan

        charge_cycles.append(charge_cycle)
        discharge_cycles.append(discharge_cycle)
        capacity_per_cycle.append(capacity)

    charge_cycles = np.array(charge_cycles)
    discharge_cycles = np.array(discharge_cycles)
    capacity_per_cycle = np.array(capacity_per_cycle)
    # print(charge_cycles)
    # print(charge_cycles.shape)
    print(discharge_cycles)
    print(discharge_cycles.shape)
    print("capacities:", capacity_per_cycle)
    import matplotlib.pyplot as plt

    # fig, ax = plt.subplots()
    # ax.plot([i for i in range(N_cycles)], capacity_per_cycle)
    # plt.show()

    print("\n\n")

    rated_capacity = np.mean(capacity_per_cycle[:5])
    N = len(discharge_cycles)
    n = int(N / 2)

    # first life
    discharge_cycles_FL = discharge_cycles[:n]
    charge_cycles_FL = charge_cycles[:n]
    capacity_per_cycle_FL = capacity_per_cycle[:n]
    state_of_health_FL = capacity_per_cycle_FL / rated_capacity
    TIECVD_FL, TIEDVD_FL = extract_features(discharge_cycles_FL, charge_cycles_FL)

    X_FL = np.column_stack([TIECVD_FL, TIEDVD_FL])
    mask_FL = np.isfinite(X_FL).all(axis=1) & np.isfinite(state_of_health_FL)
    X_FL = X_FL[mask_FL]
    y_FL = state_of_health_FL[mask_FL]

    kernel = RBF(length_scale=1.0)
    gpr = GaussianProcessRegressor(kernel=kernel, alpha=1e-4, normalize_y=True)
    gpr.fit(X_FL, y_FL)

    # second life
    discharge_cycles_SL = discharge_cycles[n:]
    charge_cycles_SL = charge_cycles[n:]
    capacity_per_cycle_SL = capacity_per_cycle[n:]
    state_of_health_SL = capacity_per_cycle_SL / rated_capacity
    TIECVD_SL, TIEDVD_SL = extract_features(discharge_cycles_SL, charge_cycles_SL)

    X_SL = np.column_stack([TIECVD_SL, TIEDVD_SL])
    mask_SL = np.isfinite(X_SL).all(axis=1) & np.isfinite(state_of_health_SL)
    X_SL = X_SL[mask_SL]
    y_SL = state_of_health_SL[mask_SL]

    cyc_SL = np.arange(n, n + len(y_SL))
    mean_pred, std_pred = gpr.predict(X_SL, return_std=True)
    rmse = np.sqrt(np.mean((mean_pred - y_SL) ** 2))
    z = 1.96
    lower = np.clip(mean_pred - z * std_pred, 0, 1)
    upper = np.clip(mean_pred + z * std_pred, 0, 1)

    plt.figure()
    plt.fill_between(cyc_SL, lower, upper, alpha=0.3)
    plt.plot(cyc_SL, y_SL)
    plt.plot(cyc_SL, mean_pred)
    plt.xlabel("Cycle Number")
    plt.ylabel("SoH")
    plt.title(f"Second-life SoH (RMSE={rmse:.4f})")
    plt.legend(["Actual", "GP Mean", "95% CI"])
    plt.show()


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
