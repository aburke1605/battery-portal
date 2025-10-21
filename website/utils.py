import numpy as np
from scipy.interpolate import interp1d

def temperature_corrected_voltage(voltage, temp_C):
    """
    Apply basic linear temperature compensation to the cell voltage
    before SoC lookup. Cold temperatures lower voltage for a given SoC.
    
    Correction factor: -1.5 mV/°C per cell below 25°C
    """
    if temp_C < 25:
        correction = (25 - temp_C) * 0.0015  # volts
        return voltage + correction
    else:
        return voltage

# Reference voltage-to-SoC curve at 25°C (Li-ion NMC, resting state)
# Format: voltage (V) : state of charge (%)
# https://www.sciencedirect.com/science/article/pii/S037877531401026X?ref=pdf_download&fr=RR-2&rr=96094dc69e31269a
voltage_soc_curve_25C = np.array([
    [4.1689, 100],
    [4.0466, 84.5],
    [3.9969, 77.0],
    [3.9637, 70.8],
    [3.9306, 64.9],
    [3.9078, 59.1],
    [3.8767, 54.1],
    [3.8539, 47.9],
    [3.8352, 41.8],
    [3.8311, 36.6],
    [3.8104, 30.0],
    [3.8000, 25.2],
    [3.7606, 20.8],
    [3.7585, 15.9],
    [3.6943, 9.5],
    [3.6632, 5.3],
    [3.6135, 2.4],
    [3.4891, 0.8],
    [3.2881, 0.0],
])
soc_interp = interp1d(voltage_soc_curve_25C[:, 0], voltage_soc_curve_25C[:, 1], bounds_error=False, fill_value=(0, 100))
def estimate_soc_per_cell(voltage, temperature):
    corrected_voltage = temperature_corrected_voltage(voltage, temperature)
    soc = np.clip(soc_interp(corrected_voltage), 0, 100)
    return soc

def date_to_string(d: int):
    if d == 0:
        return "0001-01-01"

    full_date = f"{d:06d}"

    day = full_date[:2]
    month = full_date[2:4]
    year = full_date[4:]

    return f"20{year}-{month}-{day}"

def time_to_string(t: float):
    full_time = f"{t:09.2f}"

    hour = full_time[:2]
    minute = full_time[2:4]
    second = float(full_time[4:])

    return f"{hour}:{minute}:{second:06.3f}"  # leading zero + 3 d.p. for ISO

def process_telemetry_data(data: dict):
    data["Q1"] = float(estimate_soc_per_cell(data["V1"]/10, data["T1"]/10))
    data["Q2"] = float(estimate_soc_per_cell(data["V2"]/10, data["T2"]/10))
    data["Q3"] = float(estimate_soc_per_cell(data["V3"]/10, data["T3"]/10))
    data["Q4"] = float(estimate_soc_per_cell(data["V4"]/10, data["T4"]/10))

    d = date_to_string(int(data["d"]))
    t = time_to_string(float(data["t"]))
    data["timestamp"] = f"{d}T{t}"
