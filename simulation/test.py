import numpy as np
import matplotlib.pyplot as plt


def simulate_data(
    axs,
    V_max=4.2,  # volts
    V_min=2.7,  # volts
    I=2.0,  # Amps
    N_cycles=20,
    design_capacity=2.0,  # Amp hours
    R0=0.05,  # ohms (internal resistance)
    dR=0.0005,  # ohms (per cycle increase)
    dt=1.0,  # seconds
):
    capacity = []

    def OCV(SoC):
        return V_min + (V_max - V_min) * SoC

    R_int = R0

    for cycle in range(N_cycles):
        SoC = 1.0  # start fully charged
        Q = 0.0
        t = 0.0

        Vs, Is, ts = [], [], []
        while True:
            # calculate new voltage
            V_OCV = OCV(SoC)
            V = V_OCV - I * R_int
            if V < V_min or SoC <= 0:
                break

            # update cell
            dQ = I * dt
            dC = dQ / 3600.0  # Amp hours
            SoC -= dC / design_capacity  # as fraction
            Q += dQ
            t += dt

            Vs.append(V)
            Is.append(I)
            ts.append(t)

        capacity.append(Q / 3600.0)  # Amp hours

        if cycle == 0:
            axs[0].plot(ts, Vs, marker=".")
            axs[0].plot(ts, Is, marker=".")

        R_int += dR

    axs[1].plot(range(N_cycles), capacity, marker=".")


fig, axs = plt.subplots(1, 2, figsize=(8, 3))
simulate_data(axs)
fig.savefig("plot.pdf")
