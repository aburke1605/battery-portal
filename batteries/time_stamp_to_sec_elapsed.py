import argparse
from pathlib import Path
import matplotlib.pyplot as plt

parser = argparse.ArgumentParser()
parser.add_argument("--in-file", type=str, required=True)
parser.add_argument("--output-dir", type=str, required=True)
args = parser.parse_args()

timestamps = []
elapsed_times = []
voltages = []
currents = []
temperatures = []


def convert_time_stamp_to_sec_elapsed():
    with open(args.in_file) as file:
        for line in file:
            # skip header lines
            if (
                line.strip() == "Data Log"
                or line.strip() == ""
                or line[:9] == "TimeStamp"
            ):
                continue

            timestamp, temperature, voltage, current, *_ = line.strip().split(",")
            date, time = timestamp.split(" ")
            _, _, day = date.split("-")

            # skip rows with empty cells
            if timestamp == "" or voltage == "" or current == "" or temperature == "":
                continue

            if float(current) < -500:
                continue

            timestamps.append(timestamp)
            voltages.append(float(voltage))
            currents.append(float(current))
            temperatures.append(float(temperature))

    date_1 = None
    for timestamp in timestamps:
        date, time = timestamp.split(" ")
        hours, mins, secs = time.split(":")

        # check if any days have passed
        n_days = 0
        if date_1 == None:
            date_1 = date
        elif date != date_1:
            _, _, day_1 = date_1.split("-")
            _, _, day = date.split("-")
            n_days = float(day) - float(day_1)

        # calculate time passed in seconds
        time_in_secs = (
            float(secs) + (float(mins) + float(hours) * 60) * 60 + 24 * 60 * 60 * n_days
        )
        elapsed_times.append(
            (time_in_secs - elapsed_times[0])
            if len(elapsed_times) > 0
            else time_in_secs
        )
    elapsed_times[0] = 0.0

    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    with open(f"{output_dir}/roomtemp_rel_dis_rel.csv", "w") as file:
        for i in range(len(timestamps)):
            file.write(
                f"{elapsed_times[i]:.3f},{voltages[i]},{currents[i]},{temperatures[i]}\n"
            )


def plot():
    fig, ax1 = plt.subplots()

    ax1.plot(elapsed_times, voltages, color="r", linewidth=0.5, label="Voltage")
    ax1.set_xlabel("Time elapsed (s)")
    ax1.set_ylabel("Voltage (mV)", color="r")
    ax1.tick_params(axis="y", labelcolor="r")

    ax2 = ax1.twinx()
    ax2.plot(
        elapsed_times,
        currents,
        color="b",
        linewidth=0.5,
        linestyle="--",
        label="Current",
    )
    ax2.set_ylabel("Current (mA)", color="b")
    ax2.tick_params(axis="y", labelcolor="b")

    fig.tight_layout()
    plt.savefig(f"{args.output_dir}/GPCCHEM.pdf")


def main():
    convert_time_stamp_to_sec_elapsed()
    plot()


if __name__ == "__main__":
    main()
