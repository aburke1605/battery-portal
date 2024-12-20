#include "include/I2C.h"
#include "include/utils.h"

esp_err_t i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) return err;

    err = i2c_driver_install(I2C_MASTER_NUM, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
    return err;
}

void device_scan(void) {
    ESP_LOGI("I2C", "Scanning for devices...");
    uint8_t n_devices = 0;
    for (uint8_t i = 1; i < 127; i++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (i << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(1000));
        i2c_cmd_link_delete(cmd);

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
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    esp_err_t ret;

    // first specify the register to read
    ret = i2c_master_start(cmd);
    ret = i2c_master_write_byte(cmd, (I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    ret = i2c_master_write_byte(cmd, reg, true);

    // do the reading
    ret = i2c_master_start(cmd);
    ret = i2c_master_write_byte(cmd, (I2C_ADDR << 1) | I2C_MASTER_READ, true);
    ret = i2c_master_read(cmd, data, len, I2C_MASTER_LAST_NACK);

    // clean up
    ret = i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);

    return ret;
}

uint16_t read_2byte_data(uint8_t reg) {
    esp_err_t ret;

    uint8_t data[2] = {0};
    ret = read_data(reg, data, sizeof(data));

    // is big-endian
    return (data[1] << 8) | data[0];
}

uint16_t test_read(uint8_t subclass, uint8_t offset) {
    esp_err_t ret;

    uint8_t block = get_block(offset);

    // specify the location in memory
    ret = write_byte(DATA_FLASH_CLASS, subclass);
    ret = write_byte(DATA_FLASH_BLOCK, block);

    uint8_t data[2] = {0}; // 2 bytes
    ret = read_data(BLOCK_DATA_START + offset%32, data, sizeof(data));

    // is little-endian
    uint16_t val = (data[0] << 8) | data[1];

    return val;
}

esp_err_t write_byte(uint8_t reg, uint8_t data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (!cmd) {
        ESP_LOGE("I2C", "Failed to create I2C command link.");
        return ESP_FAIL;
    }

    esp_err_t ret = i2c_master_start(cmd);
    if (ret != ESP_OK) {
        ESP_LOGE("I2C", "Failed to start I2C command: %s", esp_err_to_name(ret));
        i2c_cmd_link_delete(cmd);
        return ret;
    }

    ret |= i2c_master_write_byte(cmd, (I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    ret |= i2c_master_write_byte(cmd, reg, true);
    ret |= i2c_master_write_byte(cmd, data, true);
    ret |= i2c_master_stop(cmd);
    if (ret != ESP_OK) {
        ESP_LOGE("I2C", "Failed to build I2C write command: %s", esp_err_to_name(ret));
        i2c_cmd_link_delete(cmd);
        return ret;
    }

    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        ESP_LOGE("I2C", "Failed to execute I2C write command: %s", esp_err_to_name(ret));
    }

    return ret;
}

esp_err_t set_I2_value(uint8_t subclass, uint8_t offset, int16_t value) {
    esp_err_t ret;

    uint8_t block = get_block(offset);

    ret = write_byte(DATA_FLASH_CLASS, subclass);
    ret = write_byte(DATA_FLASH_BLOCK, block);

    // read current block data
    uint8_t block_data[32] = {0};
    ret = read_data(BLOCK_DATA_START, block_data, sizeof(block_data));
    if (ret != ESP_OK) {
        ESP_LOGE("I2C", "Failed to read Block Data.");
        return ret;
    }

    // modify the value
    block_data[offset%32]   = (value >> 8) & 0xFF; // Higher byte
    block_data[offset%32+1] =  value       & 0xFF; // Lower byte

    // Write updated block data back
    for (int i = 0; i < sizeof(block_data); i++) {
        ret = write_byte(BLOCK_DATA_START + i, block_data[i]);
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
    ret = write_byte(BLOCK_DATA_CHECKSUM, checksum);
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
    return i2c_master_write_to_device(I2C_MASTER_PORT, I2C_ADDR, data, sizeof(data), pdMS_TO_TICKS(1000));
}

esp_err_t reset_BMS() {
    esp_err_t ret;

    // Send reset subcommand
    ret = write_word(CONTROL_REG, CONTROL_RESET_SUBCMD);
    if (ret != ESP_OK) {
        ESP_LOGE("I2C", "Failed to send reset command.");
        return ret;
    }

    ESP_LOGI("I2C", "Reset command sent successfully.");

    // Delay to allow reset to complete
    vTaskDelay(pdMS_TO_TICKS(1000));

    return ESP_OK;
}
