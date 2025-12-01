import numpy as np
import matplotlib.pyplot as plt


def simulate_data(
    axs,
    V_max=4.2,  # volts
    V_min=2.7,  # volts
    I=2.0,  # Amps
    k=14,  # for OCV curve
    m=0.3,  # for OCV curve
    N_cycles=20,
    design_capacity=2.0,  # Amp hours
    R0=0.05,  # ohms (internal resistance)
    dR=0.0005,  # ohms (per cycle increase)
    a=10.0,  # for stress ageing
    SoH=1.0,  # as fraction
    dSoH=0.001,  # as fraction (per cycle decrease)
    dt=1.0,  # minutes
    V_stop=None,
    SoC_stop=0.0,
):
    capacity = []

    def OCV(SoC):
        return V_min + (V_max - V_min) * (1 / (1 + np.exp(-k * (SoC - m))))

    R_int = R0
    if V_stop == None:
        V_stop = V_min

    for cycle in range(N_cycles):
        available_capacity = design_capacity * SoH
        capacity.append(available_capacity)

        SoC = 1.0  # start fully charged
        Q = 0.0
        t = 0.0

        Vs, Is, ts = [], [], []
        while True:
            # calculate new voltage
            V_OCV = OCV(SoC)
            V = V_OCV - I * R_int
            if V < V_stop or SoC <= SoC_stop:
                break

            # update cell
            dQ = I * dt
            dC = dQ / 60.0  # Amp hours
            SoC -= dC / available_capacity  # as fraction
            Q += dQ
            t += dt

            Vs.append(V)
            Is.append(I)
            ts.append(t)

        if cycle == 0:
            axs[0].plot(ts, Vs, marker=".")
            axs[0].plot(ts, Is, marker=".")

        # age the cell for next cycle
        delivered = Q / 60.0  # Amp hours
        stress = 1 + a * (
            R_int - R0
        )  # a controls how strongly resistance speeds ageing
        SoH = max(0.0, SoH - dSoH * stress * (delivered / design_capacity))
        R_int += dR

    axs[1].plot(range(N_cycles), capacity, marker=".")


n = 1
fig, axs = plt.subplots(n, 2, figsize=(8, n * 3))
simulate_data(axs)
fig.savefig("plot.pdf")
