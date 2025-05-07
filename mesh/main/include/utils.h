#ifndef UTILS_H
#define UTILS_H

int calculate_transmission_delay(uint8_t spreading_factor, uint8_t bandwidth, uint8_t n_preamble_symbols, uint16_t payload_length, uint8_t coding_rate, bool header, bool low_data_rate_optimisation);

#endif // UTILS_H
