#include "include/config.h"
#include "include/I2C.h"
#include "include/utils.h"

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

    // Transmit the register address
    ret = i2c_master_transmit(i2c_device, &reg, 1, I2C_MASTER_TIMEOUT_MS);
    if (ret != ESP_OK) return ret;

    // Receive the data
    ret = i2c_master_receive(i2c_device, data, len, I2C_MASTER_TIMEOUT_MS);

    return ret;
}

uint16_t read_2byte_data(uint8_t reg) {
    esp_err_t ret;

    uint8_t data[2] = {0};
    ret = read_data(reg, data, sizeof(data));
    if (ret != ESP_OK) {
        // TODO: fill out
    }

    // is big-endian
    return (data[1] << 8) | data[0];
}

uint16_t test_read(uint8_t subclass, uint8_t offset) {
    esp_err_t ret;

    uint8_t block = get_block(offset);

    // specify the location in memory
    ret = write_byte(I2C_DATA_FLASH_CLASS, subclass);
    ret = write_byte(I2C_DATA_FLASH_BLOCK, block);

    uint8_t data[2] = {0}; // 2 bytes
    ret = read_data(I2C_BLOCK_DATA_START + offset%32, data, sizeof(data));
    if (ret != ESP_OK) {
        // TODO: fill out
    }

    // is little-endian
    uint16_t val = (data[0] << 8) | data[1];

    return val;
}

void read_name(uint8_t subclass, uint8_t offset, char* name) {
    esp_err_t ret;

    uint8_t block = get_block(offset);

    // specify the location in memory
    ret = write_byte(I2C_DATA_FLASH_CLASS, subclass);
    ret = write_byte(I2C_DATA_FLASH_BLOCK, block);

    char data[11] = {0}; // 11 bytes
    ret = read_data(I2C_BLOCK_DATA_START + offset%32 + 1, (uint8_t*)data, sizeof(data));
    if (ret != ESP_OK) {
        // TODO: fill out
    }

    strncpy(name, data, 10);
}

esp_err_t write_byte(uint8_t reg, uint8_t data) {
    uint8_t buffer[2] = {reg, data}; // Register address + data

    esp_err_t ret = i2c_master_transmit(i2c_device, buffer, sizeof(buffer), pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));

    if (ret != ESP_OK) {
        ESP_LOGE("I2C", "Failed to write byte to I2C device: %s", esp_err_to_name(ret));
    }

    return ret;
}

esp_err_t set_device_name(uint8_t subclass, uint8_t offset, char value[11]) {
    esp_err_t ret;

    uint8_t block = get_block(offset);
    ret = write_byte(I2C_DATA_FLASH_CLASS, subclass);
    ret = write_byte(I2C_DATA_FLASH_BLOCK, block);

    char block_data[32] = {0};
    ret = read_data(I2C_BLOCK_DATA_START, (uint8_t*)block_data, sizeof(block_data));
    if (ret != ESP_OK) {
        ESP_LOGE("I2C", "Failed to read Block Data.");
        return ret;
    }

    for (int i=0; i<10; i++) {
        block_data[offset%32+1+i] = value[i];
    }

    // Write updated block data back
    for (int i = 0; i < sizeof(block_data); i++) {
        ret = write_byte(I2C_BLOCK_DATA_START + i, block_data[i]);
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
    ret = write_byte(I2C_BLOCK_DATA_CHECKSUM, checksum);
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

    ret = write_byte(I2C_DATA_FLASH_CLASS, subclass);
    ret = write_byte(I2C_DATA_FLASH_BLOCK, block);

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
        ret = write_byte(I2C_BLOCK_DATA_START + i, block_data[i]);
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
    ret = write_byte(I2C_BLOCK_DATA_CHECKSUM, checksum);
    if (ret != ESP_OK) {
        ESP_LOGE("I2C", "Failed to write Block Data Checksum.");
        return ret;
    }

    ESP_LOGI("I2C", "Value successfully set to %d", value);

    vTaskDelay(pdMS_TO_TICKS(1000));

    return ESP_OK;
}

esp_err_t write_word(uint8_t reg, uint16_t value) {
    uint8_t data[3] = {reg, value & 0xFF, (value >> 8) & 0xFF}; // LSB first
    return i2c_master_transmit(i2c_device, data, sizeof(data), pdMS_TO_TICKS(1000));
}

esp_err_t reset_BMS() {
    esp_err_t ret;

    // Send reset subcommand
    ret = write_word(I2C_CONTROL_REG, I2C_CONTROL_RESET_SUBCMD);
    if (ret != ESP_OK) {
        ESP_LOGE("I2C", "Failed to send reset command.");
        return ret;
    }

    ESP_LOGI("I2C", "Reset command sent successfully.");

    // Delay to allow reset to complete
    vTaskDelay(pdMS_TO_TICKS(1000));

    return ESP_OK;
}
