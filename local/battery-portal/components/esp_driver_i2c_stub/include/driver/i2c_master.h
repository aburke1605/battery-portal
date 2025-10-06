#pragma once

#include "esp_err.h"
#include "esp_log.h"
#include "i2c_types.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline esp_err_t i2c_master_probe(i2c_master_bus_handle_t bus_handle, uint16_t address, int xfer_timeout_ms) {
    ESP_LOGI("[esp_driver_i2c_stub]", "i2c_master_probe called");
    return ESP_OK;
}

#ifdef __cplusplus
}
#endif
