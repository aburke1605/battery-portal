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
    dSoH=0.0005,  # as fraction (per cycle decrease)
    min_SoH=0.8,
    T_env=25.0,
    dT=0.001,
    dt=timedelta(minutes=1),
    V_dis_stop=None,
    SoC_dis_stop=0.0,
    V_chg_stop=None,
):
    global t

    capacities = []
    SoHs = []

    def OCV(SoC):
        return V_min + (V_max - V_min) * (1 / (1 + np.exp(-k * (SoC - m))))

    if V_dis_stop == None:
        V_dis_stop = V_min
    if V_chg_stop == None:
        V_chg_stop = V_max

    cycle = starting_cycle
    T = T_env
    T_max = min(T + 50.0, 90.0)
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

        first_loop = 0

        offline = False
        has_been_offline = False
        offline_timestamp = t

        SoC = 1.0  # start fully charged
        Q = 0.0
        i = 0
        while True:  #########
            # discharge loop #
            ##################

            I = rn.gauss(I_dis, 0.01)
            # calculate new voltage
            V = OCV(SoC)
            V = rn.gauss(V, 0.005 * V)
            if V < V_dis_stop or SoC <= SoC_dis_stop:
                break
            # randomly decide if unit goes offline
            if not has_been_offline and not i == first_loop:
                offline = rn.uniform(0, 1) < 0.001  # chance is 1 in 1000
                if offline:
                    has_been_offline = True
                    offline_timestamp = t
            i += 1
            if offline:
                if t - offline_timestamp > timedelta(minutes=15):
                    offline = False
            else:
                # write to file
                file.write(
                    f"{t},{T},{V*1000},{I*1000},{int(SoC*100)},{int(SoH*100)},{available_capacity},{cycle}\n"
                )
            # update cell
            dQ = abs(I) * dt.total_seconds()
            dC = dQ / 3600.0  # Amp hours
            SoC -= dC / available_capacity  # as fraction
            Q += dQ
            t += dt
            T += min((T_max - T) * dT, 0.2)
        # final write to file
        SoC = max(SoC_dis_stop, SoC)
        file.write(
            f"{t},{T},{V*1000},{I*1000},{int(SoC*100)},{int(SoH*100)},{available_capacity},{cycle}\n"
        )
        t += dt

        # calculate total amount of charge delivered during discharge segment
        delivered = Q / 3600.0  # Amp hours

        n_rest_steps = 5
        for _ in range(n_rest_steps):  ##
            #         rest loop         #
            #############################
            I = rn.gauss(0, 0.005)
            V = rn.gauss(V, 0.001 * V)
            file.write(
                f"{t},{T},{V*1000},{I*1000},{int(SoC*100)},{int(SoH*100)},{available_capacity},{cycle}\n"
            )
            t += dt
            T -= min((T - T_env) * 0.001, 0.2)

        first_loop = i

        while True:  ######
            # charge loop #
            ###############

            I = rn.gauss(I_chg, 0.01)
            # calculate new voltage
            V = OCV(SoC)
            V = rn.gauss(V, 0.005 * V)
            if V > V_chg_stop or SoC >= 1.0:
                break
            # randomly decide if unit goes offline
            if not has_been_offline and not i == first_loop:
                offline = rn.uniform(0, 1) < 0.001  # chance is 1 in 1000
                if offline:
                    has_been_offline = True
                    offline_timestamp = t
            i += 1
            if offline:
                if t - offline_timestamp > timedelta(minutes=5):
                    offline = False
            else:
                # write to file
                file.write(
                    f"{t},{T},{V*1000},{I*1000},{int(SoC*100)},{int(SoH*100)},{available_capacity},{cycle}\n"
                )
            # update cell
            dQ = I * dt.total_seconds()
            dC = dQ / 3600.0  # Amp hours
            SoC += dC / available_capacity  # as fraction
            t += dt
            T += min((T_max - T) * dT, 0.2)
        # final write to file
        SoC = min(1.0, SoC)
        file.write(
            f"{t},{T},{V*1000},{I*1000},{int(SoC*100)},{int(SoH*100)},{available_capacity},{cycle}\n"
        )
        t += dt

        for _ in range(n_rest_steps):  ##
            #         rest loop         #
            #############################
            I = rn.gauss(0, 0.005)
            V = rn.gauss(V, 0.001 * V)
            file.write(
                f"{t},{T},{V*1000},{I*1000},{int(SoC*100)},{int(SoH*100)},{available_capacity},{cycle}\n"
            )
            t += dt
            T -= min((T - T_env) * 0.001, 0.2)

        # age the cell for next cycle
        SoH = max(0.0, SoH - dSoH * (delivered / design_capacity))

        if T > 50.0:
            dSoH = max(0.005, dSoH)

        cycle += 1
        if SoH <= min_SoH or cycle - starting_cycle >= 1000:
            cycle -= 1
            break

    return cycle


def plot(
    path,
    normal_cycle_range: list,
    other_cycle_range: list = [],
    current=True,
):
    fig, axs = plt.subplots(1, 2, figsize=(16, 5))
    fig.subplots_adjust(wspace=0.3)

    ax1_L = axs[0]
    ax1_L.set_xlabel("Time [min]")
    ax1_L.set_zorder(2)
    ax1_L.patch.set_visible(False)
    if current:
        ax1_R = ax1_L.twinx()
        ax1_R.set_zorder(1)

    ax2_L = axs[1]
    ax2_L.set_xlabel("Cycle")
    ax2_R = ax2_L.twinx()

    n_cycles = len(normal_cycle_range) + len(other_cycle_range)
    norm = mcolors.Normalize(vmin=-0.5 * (n_cycles - 1), vmax=n_cycles - 1)

    SoHs, Cs = [], []
    for cycle in [*normal_cycle_range, *other_cycle_range]:
        with open(f"{path}/data_{cycle}.csv", "r") as f:
            ts, Vs, Is = [], [], []
            SoH = np.nan
            C = np.nan
            for l in f:
                if l.startswith("TimeStamp"):
                    continue
                t, T, V, I, SoC, SoH, C, _ = l.strip().split(",")
                ts.append(datetime.fromisoformat(t))
                Vs.append(float(V) / 1000.0)
                Is.append(float(I) / 1000.0)
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
            legend_cycles = [normal_cycle_range[0], normal_cycle_range[-1]]
            if len(other_cycle_range) > 0:
                legend_cycles.append(other_cycle_range[0])
                if len(other_cycle_range) > 1:
                    legend_cycles.append(other_cycle_range[-1])
            ax1_L.plot(
                t_mins,
                Vs,
                marker=".",
                color=(cm.Greens if cycle in normal_cycle_range else cm.Reds)(
                    norm(cycle)
                ),
                label=cycle if cycle in legend_cycles else None,
            )
            if current:
                ax1_R.plot(
                    t_mins,
                    Is,
                    marker=".",
                    color=cm.Purples(norm(cycle)),
                    label=cycle if cycle in legend_cycles else None,
                )
    ax1_L.legend(
        title="Voltage for cycle number:", bbox_to_anchor=(0, 1), loc="lower left"
    )
    ax1_L.set_ylabel("Voltage [V]")
    if current:
        ax1_R.legend(
            title="Current for cycle number:", bbox_to_anchor=(1, 1), loc="lower right"
        )
        ax1_R.set_ylabel("Current [A]")

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

    plt.savefig(f"{path}/plot.pdf", bbox_inches="tight")


# normal
total_n_cycles = simulate_data("data/normal")
plot("data/normal", normal_cycle_range=range(1, total_n_cycles + 1))


# low power
trigger_SoH = 0.9
n_normal_cycles = simulate_data(
    "data/low_power", min_SoH=trigger_SoH
)  # normal to start
total_n_cycles = simulate_data(
    "data/low_power", SoH=trigger_SoH, starting_cycle=n_normal_cycles + 1, I_dis=0.2
)  # then lower current
plot(
    "data/low_power",
    normal_cycle_range=range(1, n_normal_cycles + 1),
    other_cycle_range=range(n_normal_cycles + 1, total_n_cycles + 1),
    current=False,
)


# short duration
trigger_SoH = 0.95
n_normal_cycles = simulate_data(
    "data/short_duration", min_SoH=trigger_SoH
)  # normal to start
total_n_cycles = simulate_data(
    "data/short_duration",
    SoH=trigger_SoH,
    starting_cycle=n_normal_cycles + 1,
    V_dis_stop=3.8,
    dSoH=0.0025,
)  # then earlier termination
plot(
    "data/short_duration",
    normal_cycle_range=range(1, n_normal_cycles + 1),
    other_cycle_range=range(n_normal_cycles + 1, total_n_cycles + 1),
    current=False,
)


# higher temperature
trigger_SoH = 0.98
n_normal_cycles = simulate_data(
    "data/higher_temperature",
    min_SoH=trigger_SoH,
    dT=0.0001,
)  # normal to start
total_n_cycles = simulate_data(
    "data/higher_temperature",
    SoH=trigger_SoH,
    starting_cycle=n_normal_cycles + 1,
    T_env=45.0,
    dT=0.0002,
)  # then higher temperature
plot(
    "data/higher_temperature",
    normal_cycle_range=range(1, n_normal_cycles + 1),
    other_cycle_range=range(n_normal_cycles + 1, total_n_cycles + 1),
    current=False,
)
