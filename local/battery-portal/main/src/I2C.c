#include "include/I2C.h"

#include "include/config.h"

#include "esp_log.h"
#include "driver/i2c_master.h"

static i2c_master_bus_handle_t i2c_bus = NULL;
static i2c_master_dev_handle_t i2c_device = NULL;

static const char* TAG = "I2C";

esp_err_t check_device() {
    esp_err_t ret = i2c_master_probe(i2c_bus, I2C_ADDR, I2C_MASTER_TIMEOUT_MS);
    if (ret != ESP_OK) ESP_LOGE(TAG, "I2C device not found at address 0x%x.", I2C_ADDR);

    return ret;
}

esp_err_t read_SBS_data(uint8_t reg, uint8_t* data, size_t data_size) {
    esp_err_t ret = check_device();
    if (ret != ESP_OK) return ret;

    if (!data || data_size == 0) {
        ESP_LOGE(TAG, "Invalid data buffer or length.");
        return ESP_ERR_INVALID_ARG;
    }

    // transmit the register address
    ret = i2c_master_transmit(i2c_device, &reg, 1, I2C_MASTER_TIMEOUT_MS);
    if (ret != ESP_OK) return ret;

    // receive the response data
    ret = i2c_master_receive(i2c_device, data, data_size, I2C_MASTER_TIMEOUT_MS);

    return ret;
}
