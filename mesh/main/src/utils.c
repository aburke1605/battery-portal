#include <cJSON.h>

#include "include/utils.h"

int calculate_transmission_delay(uint8_t spreading_factor, uint8_t bandwidth, uint8_t n_preamble_symbols, uint16_t payload_length, uint8_t coding_rate, bool header, bool low_data_rate_optimisation) {
    float frequencies[] = {7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125.0, 250.0, 500.0};
    if (bandwidth < 0 || bandwidth >= sizeof(frequencies) / sizeof(frequencies[0])) {
        ESP_LOGW("LoRa", "Undefined bandwidth key! Returning to default...");
        bandwidth = 7;
    }

    float t_sym = pow(2, (float)spreading_factor) / (frequencies[bandwidth]);
    if (VERBOSE) printf("symbol time: %f ms\n", t_sym);

    float n_payload_symbols = ceil((8*payload_length - 4*spreading_factor + 28 + 16 - (header?20:0)) / (spreading_factor - (low_data_rate_optimisation?2:0)) / 4);
    n_payload_symbols *= (coding_rate + 4);
    if (n_payload_symbols < 0) {
        n_payload_symbols = 0;
        printf("n_payload_symbols is 0!\n");
    }
    if (VERBOSE) printf("N payload symbols: %f\n", n_payload_symbols);

    float t_air = (4.25 + (float)n_preamble_symbols + n_payload_symbols) * t_sym;
    if (VERBOSE) printf("time on air: %f ms\n", t_air);

    float n_payloads_per_hour = 36e3 / t_air;
    if (VERBOSE) printf("number of payloads per hour: %f\n", n_payloads_per_hour);

    float time_delay_seconds = pow(60, 2) / n_payloads_per_hour;
    if (VERBOSE) printf("delay between messages: %f s\n", time_delay_seconds);

    return (int)(time_delay_seconds * 1e3);
}
