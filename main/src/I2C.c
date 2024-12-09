#include "include/I2C.h"

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

uint16_t read_2byte_data(int REG_ADDR) {

uint16_t read_BL() {
    esp_err_t ret;

    // specify the location in memory
    ret = write_byte(DATA_FLASH_CLASS, CONFIGURATION_DISCHARGE_SUBCLASS_ID);
    ret = write_byte(DATA_FLASH_BLOCK, FIRST_DATA_BLOCK);

    uint8_t data[2] = {0}; // 2 bytes
    ret = read_data(BLOCK_DATA_START + BL_OFFSET, data, sizeof(data));

    // is little-endian
    return (data[0] << 8) | data[1];
}

    // Send command to request the state of charge register
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, REG_ADDR, true);
    i2c_master_stop(cmd);

    esp_err_t err = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    if (err != ESP_OK) {
        ESP_LOGE("I2C", "Error in I2C transmission: %s", esp_err_to_name(err));
        return 0;
    }

    // Request 2 bytes from the device
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (I2C_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, 2, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);

    err = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    if (err != ESP_OK) {
        ESP_LOGE("I2C", "Error in I2C read: %s", esp_err_to_name(err));
        return 0;
    }

    uint16_t ret = (data[1] << 8) | data[0];
    return ret;
}

esp_err_t set_BL_voltage_threshold(int16_t BL) {
    esp_err_t ret;

    ret = write_byte(DATA_FLASH_CLASS, CONFIGURATION_DISCHARGE_SUBCLASS_ID);    
    ret = write_byte(DATA_FLASH_BLOCK, FIRST_DATA_BLOCK);

    uint16_t value = read_BL();
    printf("\nvalue at %02X+%02X = 0x%04X (%d mV)\n", BLOCK_DATA_START, BL_OFFSET, value, value);

    // Read current block data
    uint8_t block_data[32] = {0};
    ret = read_data(BLOCK_DATA_START, block_data, sizeof(block_data));
    if (ret != ESP_OK) {
        ESP_LOGE("I2C", "Failed to read Block Data.");
        return ret;
    }
    
    // Modify the Undertemperature Charge value
    block_data[BL_OFFSET]   = (BL >> 8) & 0xFF; // Higher byte (0x0D)
    block_data[BL_OFFSET+1] =  BL       & 0xFF; // Lower byte (0xAC)

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

    ESP_LOGI("I2C", "BL voltage threshold successfully set to %d mV", BL);
    return ESP_OK;
}
