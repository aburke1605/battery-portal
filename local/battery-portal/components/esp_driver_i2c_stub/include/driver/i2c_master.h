#pragma once

#include "esp_err.h"
#include "esp_log.h"
#include "i2c_types.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline esp_err_t i2c_master_transmit(i2c_master_dev_handle_t i2c_dev, const uint8_t *write_buffer, size_t write_size, int xfer_timeout_ms) {
    ESP_LOGI("[esp_driver_i2c_stub]", "i2c_master_transmit called");
    return ESP_OK;
}

static inline esp_err_t i2c_master_receive(i2c_master_dev_handle_t i2c_dev, uint8_t *read_buffer, size_t read_size, int xfer_timeout_ms) {
    ESP_LOGI("[esp_driver_i2c_stub]", "i2c_master_receive called");
    return ESP_OK;
}

static inline esp_err_t i2c_master_probe(i2c_master_bus_handle_t bus_handle, uint16_t address, int xfer_timeout_ms) {
    ESP_LOGI("[esp_driver_i2c_stub]", "i2c_master_probe called");
    return ESP_OK;
}

#ifdef __cplusplus
}
#endif
