import numpy as np
import random as rn
import matplotlib.pyplot as plt
import matplotlib.cm as cm
import matplotlib.colors as mcolors
from pathlib import Path
from datetime import datetime, timedelta

t = datetime.now().replace(second=0, microsecond=0) - timedelta(
    days=365 * 5
)  # take off 5 years


def simulate_data(
    path,
    V_max=4.2,  # volts
    V_min=2.7,  # volts
    I_dis=-2.0,  # Amps
    I_chg=5.0,  # Amps
    k=14,  # for OCV curve
    m=0.3,  # for OCV curve
    design_capacity=2.0,  # Amp hours
    starting_cycle=1,
    SoH=1.0,  # as fraction
    dSoH=0.0001,  # as fraction (per cycle decrease)
    min_SoH=0.8,
    dt=timedelta(minutes=1),
    V_dis_stop=None,
    SoC_dis_stop=0.0,
    V_chg_stop=None,
):
    global t

    plot_data = {"t": [], "V": [], "I": []}

    capacities = []
    SoHs = []

    def OCV(SoC):
        return V_min + (V_max - V_min) * (1 / (1 + np.exp(-k * (SoC - m))))

    if V_dis_stop == None:
        V_dis_stop = V_min
    if V_chg_stop == None:
        V_chg_stop = V_max

    cycle = starting_cycle
    while True:  #####
        # cycle loop #
        ##############

        Path(path).mkdir(parents=True, exist_ok=True)
        file = open(f"{path}/data_{cycle}.csv", "w")
        file.write(
            "TimeStamp,Temperature,Voltage,Current,Relative State of Charge,State of Health,Capacity,Cycle\n"
        )

        available_capacity = design_capacity * SoH
        capacities.append(available_capacity)
        SoHs.append(SoH)

        ts, Vs, Is = [], [], []

        offline = False
        has_been_offline = False
        offline_timestamp = t

        SoC = 1.0  # start fully charged
        Q = 0.0
        while True:  #########
            # discharge loop #
            ##################

            # calculate new voltage
            V = OCV(SoC)
            if V < V_dis_stop or SoC <= SoC_dis_stop:
                break
            # randomly decide if unit goes offline
            if not has_been_offline:
                offline = rn.uniform(0, 1) < 0.001  # chance is 1 in 1000
                if offline:
                    has_been_offline = True
                    offline_timestamp = t
            if offline:
                Vs.append(np.nan)
                Is.append(np.nan)
                if t - offline_timestamp > timedelta(minutes=15):
                    offline = False
            else:
                Vs.append(V)
                Is.append(I_dis)
                # write to file
                file.write(
                    f"{t},25.0,{V},{I_dis},{int(SoC*100)},{int(SoH*100)},{available_capacity},{cycle}\n"
                )
            ts.append(t)
            # update cell
            dQ = abs(I_dis) * dt.total_seconds()
            dC = dQ / 3600.0  # Amp hours
            SoC -= dC / available_capacity  # as fraction
            Q += dQ
            t += dt
        # final write to file
        SoC = max(SoC_dis_stop, SoC)
        file.write(
            f"{t},25.0,{V},{I_dis},{int(SoC*100)},{int(SoH*100)},{available_capacity},{cycle}\n"
        )

        # calculate total amount of charge delivered during discharge segment
        delivered = Q / 3600.0  # Amp hours

        n_rest_steps = 5
        for _ in range(n_rest_steps):  ##
            #         rest loop         #
            #############################
            Vs.append(V)
            Is.append(0)
            ts.append(t)
            file.write(
                f"{t},25.0,{V},0,{int(SoC*100)},{int(SoH*100)},{available_capacity},{cycle}\n"
            )
            t += dt

        while True:  ######
            # charge loop #
            ###############

            # calculate new voltage
            V = OCV(SoC)
            if V > V_chg_stop or SoC >= 1.0:
                break
            # randomly decide if unit goes offline
            if not has_been_offline:
                offline = rn.uniform(0, 1) < 0.001  # chance is 1 in 1000
                if offline:
                    has_been_offline = True
                    offline_timestamp = t
            if offline:
                Vs.append(np.nan)
                Is.append(np.nan)
                if t - offline_timestamp > timedelta(minutes=5):
                    offline = False
            else:
                Vs.append(V)
                Is.append(I_chg)
                # write to file
                file.write(
                    f"{t},25.0,{V},{I_chg},{int(SoC*100)},{int(SoH*100)},{available_capacity},{cycle}\n"
                )
            ts.append(t)
            # update cell
            dQ = I_chg * dt.total_seconds()
            dC = dQ / 3600.0  # Amp hours
            SoC += dC / available_capacity  # as fraction
            t += dt
        # final write to file
        SoC = min(1.0, SoC)
        file.write(
            f"{t},25.0,{V},{I_chg},{int(SoC*100)},{int(SoH*100)},{available_capacity},{cycle}\n"
        )

        for _ in range(n_rest_steps):  ##
            #         rest loop         #
            #############################
            Vs.append(V)
            Is.append(0)
            ts.append(t)
            file.write(
                f"{t},25.0,{V},0,{int(SoC*100)},{int(SoH*100)},{available_capacity},{cycle}\n"
            )
            t += dt

        # age the cell for next cycle
        SoH = max(0.0, SoH - dSoH * (delivered / design_capacity))

        plot_data["t"].append(ts)
        plot_data["V"].append(Vs)
        plot_data["I"].append(Is)

        cycle += 1
        if SoH <= min_SoH or cycle >= 1000:
            break

    return cycle


def plot(
    path,
    n_cycles,
    current=True,
    voltage_colormap=cm.Greens,
):
    fig, axs = plt.subplots(1, 2, figsize=(16, 5))
    fig.subplots_adjust(wspace=0.3)

    ax1_L = axs[0]
    ax1_L.set_xlabel("Time [min]")
    ax1_R = ax1_L.twinx()
    ax1_L.set_zorder(2)
    ax1_L.patch.set_visible(False)
    ax1_R.set_zorder(1)

    ax2_L = axs[1]
    ax2_L.set_xlabel("Cycle")
    ax2_R = ax2_L.twinx()

    norm = mcolors.Normalize(vmin=-0.5 * (n_cycles - 1), vmax=n_cycles - 1)

    SoHs, Cs = [], []
    for i in range(n_cycles):
        with open(f"{path}/data_{i}.csv", "r") as f:
            ts, Vs, Is = [], [], []
            SoH = np.nan
            C = np.nan
            for l in f:
                if l.startswith("TimeStamp"):
                    continue
                t, T, V, I, SoC, SoH, C, cycle = l.strip().split(",")
                ts.append(datetime.fromisoformat(t))
                Vs.append(float(V))
                Is.append(float(I))
            SoHs.append(float(SoH))
            Cs.append(float(C))

            t_mins = np.array([(t - ts[0]).total_seconds() / 60.0 for t in ts])
            Vs = np.array(Vs)
            Is = np.array(Is)

            # break line where gap > 1 minute
            gaps = np.diff(t_mins) > 1
            break_idx = np.where(gaps)[0] + 1
            t_mins[break_idx] = np.nan
            Vs[break_idx] = np.nan

            # plot voltages and currents
            ax1_L.plot(
                t_mins,
                Vs,
                marker=".",
                color=voltage_colormap(norm(i)),
                label=i + 1 if i == 0 or i == n_cycles - 1 else None,
            )
            if current:
                ax1_R.plot(
                    t_mins,
                    Is,
                    marker=".",
                    color=cm.Purples(norm(i)),
                    label=i + 1 if i == 0 or i == n_cycles - 1 else None,
                )
    ax1_L.legend(
        title="Voltage for cycle number:", bbox_to_anchor=(0, 0.5), loc="lower left"
    )
    ax1_L.set_ylabel("Voltage [V]", color=cm.Greens(norm(n_cycles - 1)))
    if current:
        ax1_R.legend(
            title="Current for cycle number:", bbox_to_anchor=(0, 0.5), loc="upper left"
        )
        ax1_R.set_ylabel("Current [A]", color=cm.Purples(norm(n_cycles - 1)))

    # plot state of health and capacity
    x = np.arange(1, n_cycles + 1, 1, dtype=int)
    ax2_L.plot(
        x,
        Cs,
        marker=".",
        color=cm.Blues(norm(n_cycles - 1)),
    )
    ax2_R.plot(
        x,
        SoHs,
        marker=".",
        color=cm.Oranges(norm(n_cycles - 1)),
        linestyle="--",
    )
    ax2_L.set_ylim(0, max(Cs))
    ax2_R.set_ylim(80.0, 100.0)
    ax2_L.set_ylabel("Capacity [Ah]", color=cm.Blues(norm(n_cycles - 1)))
    ax2_R.set_ylabel("State of Health [%]", color=cm.Oranges(norm(n_cycles - 1)))

    plt.savefig(f"{path}/plot.pdf")


simulate_data("data/normal")
n_cycles = simulate_data("data/low_power", min_SoH=0.9)
simulate_data("data/low_power", SoH=0.9, starting_cycle=n_cycles, I_dis=0.2)

plot("data/normal", 3, current=False)
plot("data/low_power", 3, current=False, voltage_colormap=cm.Reds)
