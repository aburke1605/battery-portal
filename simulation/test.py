import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path


def simulate_data(
    ax,
    path,
    V_max=4.2,  # volts
    V_min=2.7,  # volts
    I=2.0,  # Amps
    k=14,  # for OCV curve
    m=0.3,  # for OCV curve
    design_capacity=2.0,  # Amp hours
    R0=0.05,  # ohms (internal resistance)
    dR=0.0005,  # ohms (per cycle increase)
    a=10.0,  # for stress ageing
    SoH=1.0,  # as fraction
    dSoH=0.0001,  # as fraction (per cycle decrease)
    dt=1.0,  # minutes
    V_stop=None,
    SoC_stop=0.0,
):
    capacities = []
    SoHs = []

    def OCV(SoC):
        return V_min + (V_max - V_min) * (1 / (1 + np.exp(-k * (SoC - m))))

    R_int = R0
    if V_stop == None:
        V_stop = V_min - 0.1

    i = 0
    while True:
        Path(path).mkdir(parents=True, exist_ok=True)
        file = open(f"{path}/data_{i}.csv", "w")
        file.write(
            "TimeStamp,Temperature,Voltage,Current,Relative State of Charge,State of Health,Cycle\n"
        )

        available_capacity = design_capacity * SoH
        capacities.append(available_capacity)
        SoHs.append(SoH)

        SoC = 1.0  # start fully charged
        Q = 0.0
        t = 0.0

        ts, Vs, Is = [], [], []
        while True:
            # calculate new voltage
            V_OCV = OCV(SoC)
            V = V_OCV - I * R_int
            if V < V_stop or SoC <= SoC_stop:
                break

            Vs.append(V)
            Is.append(I)
            ts.append(t)

            file.write(f"{t},25.0,{V},{I},{int(SoC*100)},{int(SoH*100)},{i}\n")

            # update cell
            dQ = I * dt
            dC = dQ / 60.0  # Amp hours
            SoC -= dC / available_capacity  # as fraction
            Q += dQ
            t += dt

        file.write(f"{t},25.0,{V},{I},{int(SoC*100)},{int(SoH*100)},{i}\n")

        # age the cell for next cycle
        delivered = Q / 60.0  # Amp hours
        stress = 1 + a * (
            R_int - R0
        )  # a controls how strongly resistance speeds ageing
        SoH = max(0.0, SoH - dSoH * stress * (delivered / design_capacity))
        R_int += dR

        if i == 0:
            ax[0].plot(ts, Vs, marker=".")
            for V_lim in [V_min, V_max]:
                ax[0].hlines(
                    V_lim, ts[0], ts[-1], linestyle="--", linewidth=0.5, color="k"
                )
            ax[0].plot(ts, Is, marker=".")

        i += 1
        if SoH <= 0.8 or i >= 1000:
            break

    ax[1].plot(range(len(capacities)), capacities, marker=".")
    ax[1].plot(range(len(SoHs)), SoHs, marker=".")
    ax[1].set_ylim(0, max(1.0, design_capacity))


n = 1
fig, axs = plt.subplots(n, 2, figsize=(8, n * 3))
simulate_data(axs, "data")
fig.savefig("plot.pdf")
