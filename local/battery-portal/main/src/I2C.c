#include "include/I2C.h"

#include "include/config.h"

#include <stdbool.h>
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "driver/i2c_types.h"
#include "freertos/FreeRTOS.h"

static i2c_master_bus_handle_t i2c_bus = NULL;
static i2c_master_dev_handle_t i2c_device = NULL;

static const char* TAG = "I2C";

esp_err_t i2c_master_init(void) {
    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_NUM,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 0,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t err = i2c_new_master_bus(&bus_cfg, &i2c_bus);
    if (err != ESP_OK) return err;

    i2c_device_config_t dev_cfg = {
        .device_address = I2C_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ
    };
    err |= i2c_master_bus_add_device(i2c_bus, &dev_cfg, &i2c_device);

    return err;
}

esp_err_t check_device() {
    esp_err_t ret = i2c_master_probe(i2c_bus, I2C_ADDR, I2C_MASTER_TIMEOUT_MS);
    if (ret != ESP_OK) ESP_LOGE(TAG, "I2C device not found at address 0x%x.", I2C_ADDR);

    return ret;
}

void device_scan(void) {
    ESP_LOGI(TAG, "Scanning for devices...");
    uint8_t n_devices = 0;
    for (uint8_t i = 1; i < 127; i++) {
        esp_err_t ret = i2c_master_probe(i2c_bus, i, pdMS_TO_TICKS(1000));

        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "I2C device found at address 0x%02X", i);
            n_devices++;
        }
    }

    if (n_devices == 0) ESP_LOGW(TAG, "No devices found.");
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
