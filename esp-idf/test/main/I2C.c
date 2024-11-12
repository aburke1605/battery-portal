#include <driver/i2c.h>

#define I2C_ADDR CONFIG_I2C_ADDR

#define I2C_MASTER_SDA_IO CONFIG_I2C_MASTER_SDA_PIN
#define I2C_MASTER_SCL_IO CONFIG_I2C_MASTER_SCL_PIN
#define I2C_MASTER_NUM I2C_NUM_0 // I2C_NUM_0 is the first I2C bus available on esp32
#define I2C_FREQ_HZ CONFIG_I2C_FREQ_HZ

#define I2C_MASTER_TX_BUF_DISABLE 0 // I2C master doesn't need buffer
#define I2C_MASTER_RX_BUF_DISABLE 0 // I2C master doesn't need buffer

// Function to initialize I2C master
esp_err_t i2c_master_init() {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ,
    };

    esp_err_t ret = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE("I2C", "I2C config failed");
        return ret;
    }
    ret = i2c_driver_install(I2C_MASTER_NUM, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
    return ret;
}


esp_err_t i2c_read_charge(uint8_t *data, size_t length) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    esp_err_t ret;

    // Start I2C communication and write the register address (if required)
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    // Send the register address to read from (optional)
    i2c_master_write_byte(cmd, 0x2C, true);  // address for SOC
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(1000));  // Execute the command
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        ESP_LOGE("I2C", "Failed to write address");
        return ret;
    }

    // Start reading data
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (I2C_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, length, I2C_MASTER_ACK);
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(1000));  // Execute the read command
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        ESP_LOGE("I2C", "Failed to read data");
    }

    return ret;
}
