#include "include/I2C.h"

#include "include/config.h"
#include "include/utils.h"

#include <esp_log.h>
#include <driver/i2c_master.h>

static i2c_master_bus_handle_t i2c_bus = NULL;
static i2c_master_dev_handle_t i2c_device = NULL;

esp_err_t i2c_master_init(void) {
    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_NUM,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7, // default for glitch filtering
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
    if (ret != ESP_OK) ESP_LOGE("I2C", "I2C device not found.");

    return ret;
}

void device_scan(void) {
    ESP_LOGI("I2C", "Scanning for devices...");
    uint8_t n_devices = 0;
    for (uint8_t i = 1; i < 127; i++) {
        esp_err_t ret = i2c_master_probe(i2c_bus, i, pdMS_TO_TICKS(1000));

        if (ret == ESP_OK) {
            ESP_LOGI("I2C", "I2C device found at address 0x%02X", i);
            n_devices++;
        }
    }

    if (n_devices == 0) ESP_LOGW("I2C", "No devices found.");
}

esp_err_t read_data(uint8_t reg, uint8_t* data, size_t len) {
    if (!data || len == 0) {
        ESP_LOGE("I2C", "Invalid data buffer or length.");
        return ESP_ERR_INVALID_ARG;
        // TODO: error check the rest of this function
    }
    esp_err_t ret;

    ret = check_device();
    if (ret != ESP_OK) return ret;

    // Transmit the register address
    ret = i2c_master_transmit(i2c_device, &reg, 1, I2C_MASTER_TIMEOUT_MS);
    if (ret != ESP_OK) return ret;

    // Receive the data
    ret = i2c_master_receive(i2c_device, data, len, I2C_MASTER_TIMEOUT_MS);

    return ret;
}

esp_err_t write_data(uint8_t reg, uint32_t data, size_t n_btyes) {
    esp_err_t ret;

    ret = check_device();
    if (ret != ESP_OK) return ret;

    uint8_t buffer[1 + n_btyes] = {};
    buffer[0] = reg;
    // LSB first
    for (size_t i = 0; i < n_btyes; i++) {
        buffer[i+1] = (data >> (8 * i)) & 0xFF; // using `x & 0xFF` ensures only LSB of x is used
    }

    ret = i2c_master_transmit(i2c_device, buffer, sizeof(buffer), pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));

    if (ret != ESP_OK) {
        ESP_LOGE("I2C", "Failed to write byte to I2C device: %s", esp_err_to_name(ret));
    }

    return ret;
}

void read_bytes(uint8_t subclass, uint8_t offset, uint8_t* data, size_t n_bytes) {
    esp_err_t ret;

    if (subclass == 0) {
        // request is a read of simple memory data
        ret = read_data(offset, data, n_bytes);
    } else {
        uint8_t block = get_block(offset);

        // specify the location in memory
        ret = write_data(I2C_DATA_FLASH_CLASS, subclass, 1);
        ret = write_data(I2C_DATA_FLASH_BLOCK, block, 1);

        ret = read_data(I2C_BLOCK_DATA_START + offset%32, data, n_bytes);
    }

    if (ret != ESP_OK) {
        // TODO: fill out
    }
}

esp_err_t set_device_name(uint8_t subclass, uint8_t offset, char value[11]) {
    esp_err_t ret;

    uint8_t block = get_block(offset);
    ret = write_data(I2C_DATA_FLASH_CLASS, subclass, 1);
    ret = write_data(I2C_DATA_FLASH_BLOCK, block, 1);

    char block_data[32] = {0};
    ret = read_data(I2C_BLOCK_DATA_START, (uint8_t*)block_data, sizeof(block_data));
    if (ret != ESP_OK) {
        ESP_LOGE("I2C", "Failed to read Block Data.");
        return ret;
    }

    for (int i=0; i<10; i++) {
        block_data[offset%32+i] = value[i];
    }

    // Write updated block data back
    for (int i = 0; i < sizeof(block_data); i++) {
        ret = write_data(I2C_BLOCK_DATA_START + i, block_data[i], 1);
        if (ret != ESP_OK) {
            ESP_LOGE("I2C", "Failed to write Block Data at index %d.", i);
            return ret;
        }
    }

    // Calculate new checksum
    uint8_t checksum = 0;
    for (int i = 0; i < sizeof(block_data); i++) {
        checksum += block_data[i];
    }
    checksum = 0xFF - checksum;

    // Write new checksum
    ret = write_data(I2C_BLOCK_DATA_CHECKSUM, checksum, 1);
    if (ret != ESP_OK) {
        ESP_LOGE("I2C", "Failed to write Block Data Checksum.");
        return ret;
    }
    ESP_LOGI("I2C", "Device name successfully set to %s", value);

    vTaskDelay(pdMS_TO_TICKS(1000));

    return ESP_OK;
}

esp_err_t set_I2_value(uint8_t subclass, uint8_t offset, int16_t value) {
    esp_err_t ret;

    uint8_t block = get_block(offset);

    ret = write_data(I2C_DATA_FLASH_CLASS, subclass, 1);
    ret = write_data(I2C_DATA_FLASH_BLOCK, block, 1);

    // read all data in current block (32 bytes)
    uint8_t block_data[32] = {0};
    ret = read_data(I2C_BLOCK_DATA_START, block_data, sizeof(block_data));
    if (ret != ESP_OK) {
        ESP_LOGE("I2C", "Failed to read Block Data.");
        return ret;
    }

    // modify the value of interest only in sub-block (2 bytes)
    block_data[offset%32]   = (value >> 8) & 0xFF; // Higher byte
    block_data[offset%32+1] =  value       & 0xFF; // Lower byte

    // Write updated block data back
    for (int i = 0; i < sizeof(block_data); i++) {
        ret = write_data(I2C_BLOCK_DATA_START + i, block_data[i], 1);
        if (ret != ESP_OK) {
            ESP_LOGE("I2C", "Failed to write Block Data at index %d.", i);
            return ret;
        }
    }

    // Calculate new checksum
    uint8_t checksum = 0;
    for (int i = 0; i < sizeof(block_data); i++) {
        checksum += block_data[i];
    }
    checksum = 0xFF - checksum;

    // Write new checksum
    ret = write_data(I2C_BLOCK_DATA_CHECKSUM, checksum, 1);
    if (ret != ESP_OK) {
        ESP_LOGE("I2C", "Failed to write Block Data Checksum.");
        return ret;
    }

    ESP_LOGI("I2C", "Value successfully set to %d", value);

    vTaskDelay(pdMS_TO_TICKS(1000));

    return ESP_OK;
}
