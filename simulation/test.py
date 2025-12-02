import numpy as np
import matplotlib.pyplot as plt
import matplotlib.cm as cm
import matplotlib.colors as mcolors
from pathlib import Path


def simulate_data(
    ax,
    path,
    V_max=4.2,  # volts
    V_min=2.7,  # volts
    I_dis=-2.0,  # Amps
    I_chg=5.0,  # Amps
    k=14,  # for OCV curve
    m=0.3,  # for OCV curve
    design_capacity=2.0,  # Amp hours
    SoH=1.0,  # as fraction
    dSoH=0.0001,  # as fraction (per cycle decrease)
    dt=1.0,  # minutes
    V_dis_stop=None,
    SoC_dis_stop=0.0,
    V_chg_stop=None,
):
    plot_data = {"t": [], "V": [], "I": []}

    capacities = []
    SoHs = []

    def OCV(SoC):
        return V_min + (V_max - V_min) * (1 / (1 + np.exp(-k * (SoC - m))))

    if V_dis_stop == None:
        V_dis_stop = V_min
    if V_chg_stop == None:
        V_chg_stop = V_max

    i = 0
    while True:  #####
        # cycle loop #
        ##############

        Path(path).mkdir(parents=True, exist_ok=True)
        file = open(f"{path}/data_{i}.csv", "w")
        file.write(
            "TimeStamp,Temperature,Voltage,Current,Relative State of Charge,State of Health,Cycle\n"
        )

        available_capacity = design_capacity * SoH
        capacities.append(available_capacity)
        SoHs.append(SoH)

        ts, Vs, Is = [], [], []

        t = 0.0

        SoC = 1.0  # start fully charged
        Q = 0.0
        while True:  #########
            # discharge loop #
            ##################

            # calculate new voltage
            V = OCV(SoC)
            if V < V_dis_stop or SoC <= SoC_dis_stop:
                break
            # append plotting data
            Vs.append(V)
            Is.append(I_dis)
            ts.append(t)
            # write to file
            file.write(f"{t},25.0,{V},{I_dis},{int(SoC*100)},{int(SoH*100)},{i}\n")
            # update cell
            dQ = abs(I_dis) * dt
            dC = dQ / 60.0  # Amp hours
            SoC -= dC / available_capacity  # as fraction
            Q += dQ
            t += dt
        # final write to file
        file.write(f"{t},25.0,{V},{I_dis},{int(SoC*100)},{int(SoH*100)},{i}\n")

        # calculate total amount of charge delivered during discharge segment
        delivered = Q / 60.0  # Amp hours

        n_rest_steps = 5
        for _ in range(n_rest_steps):  ##
            #         rest loop         #
            #############################
            Vs.append(V)
            Is.append(0)
            ts.append(t)
            file.write(f"{t},25.0,{V},0,{int(SoC*100)},{int(SoH*100)},{i}\n")
            t += dt

        while True:  ######
            # charge loop #
            ###############

            # calculate new voltage
            V = OCV(SoC)
            if V > V_chg_stop or SoC >= 1.0:
                break
            # append plotting data
            Vs.append(V)
            Is.append(I_chg)
            ts.append(t)
            # write to file
            file.write(f"{t},25.0,{V},{I_chg},{int(SoC*100)},{int(SoH*100)},{i}\n")
            # update cell
            dQ = I_chg * dt
            dC = dQ / 60.0  # Amp hours
            SoC += dC / available_capacity  # as fraction
            t += dt
        # final write to file
        file.write(f"{t},25.0,{V},{I_chg},{int(SoC*100)},{int(SoH*100)},{i}\n")

        for _ in range(n_rest_steps):  ##
            #         rest loop         #
            #############################
            Vs.append(V)
            Is.append(0)
            ts.append(t)
            file.write(f"{t},25.0,{V},0,{int(SoC*100)},{int(SoH*100)},{i}\n")
            t += dt

        # age the cell for next cycle
        SoH = max(0.0, SoH - dSoH * (delivered / design_capacity))

        plot_data["t"].append(ts)
        plot_data["V"].append(Vs)
        plot_data["I"].append(Is)

        i += 1
        if SoH <= 0.8 or i >= 1000:
            break

    n_plots = len(plot_data["t"])
    norm = mcolors.Normalize(vmin=-0.5 * (n_plots - 1), vmax=n_plots - 1)
    ax1 = ax[0]
    ax1.set_xlabel("Time [min]")
    ax2 = ax1.twinx()
    ax1.set_zorder(2)
    ax1.patch.set_visible(False)
    ax2.set_zorder(1)
    for i in range(n_plots):
        ax1.plot(
            plot_data["t"][i],
            plot_data["V"][i],
            marker=".",
            color=cm.Greens(norm(i)),
            label=i + 1 if i == 0 or i == n_plots - 1 else None,
        )
        ax2.plot(
            plot_data["t"][i],
            plot_data["I"][i],
            marker=".",
            color=cm.Purples(norm(i)),
            label=i + 1 if i == 0 or i == n_plots - 1 else None,
        )
    for V_lim in [V_min, V_max]:
        ax1.hlines(V_lim, *ax1.get_xlim(), linestyle="--", linewidth=0.5, color="k")
    ax1.legend(title="Voltage for cycle number:", loc="lower right")
    ax2.legend(title="Current for cycle number:", loc="lower left")
    ax1.set_ylabel("Voltage [V]", color=cm.Greens(norm(n_plots - 1)))
    ax2.set_ylabel("Current [A]", color=cm.Purples(norm(n_plots - 1)))

    ax3 = ax[1]
    ax3.set_xlabel("Cycle")
    ax4 = ax3.twinx()
    ax3.plot(
        range(len(capacities)),
        capacities,
        marker=".",
        color=cm.Blues(norm(n_plots - 1)),
    )
    ax4.plot(
        range(len(SoHs)),
        SoHs,
        marker=".",
        color=cm.Oranges(norm(n_plots - 1)),
        linestyle="--",
    )
    ax3.set_ylim(0, design_capacity)
    ax4.set_ylim(0, 1.0)
    ax3.set_ylabel("Capacity [Ah]", color=cm.Blues(norm(n_plots - 1)))
    ax4.set_ylabel("State of Health [%]", color=cm.Oranges(norm(n_plots - 1)))


n = 1
fig, axs = plt.subplots(n, 2, figsize=(16, n * 5))
simulate_data(axs, "data", dSoH=0.025)
fig.subplots_adjust(wspace=0.3)
fig.savefig("plot.pdf")
