#include <driver/i2c.h>

#define I2C_ADDR                   CONFIG_I2C_ADDR           // 7-bit I2C address of the battery
#define I2C_MASTER_SDA_IO          CONFIG_I2C_MASTER_SDA_PIN // GPIO number for I2C master data
#define I2C_MASTER_SCL_IO          CONFIG_I2C_MASTER_SCL_PIN // GPIO number for I2C master clock
#define I2C_MASTER_FREQ_HZ         CONFIG_I2C_FREQ_HZ        // I2C master clock frequency
#define I2C_MASTER_NUM             I2C_NUM_0                 // I2C port number for master dev

#define STATE_OF_CHARGE_REG        0x2C                      // Register address for StateOfCharge

#define I2C_MASTER_TIMEOUT_MS      1000                      // time delay to allow for BMS response
#define I2C_MASTER_TX_BUF_DISABLE  0                         // I2C master doesn't need buffer
#define I2C_MASTER_RX_BUF_DISABLE  0                         // I2C master doesn't need buffer


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

    printf("I2C scanner scanning...\n");
    for (uint8_t i = 1; i < 127; i++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (i << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(1000));
        i2c_cmd_link_delete(cmd);

        if (ret == ESP_OK) {
            printf("I2C device found at address 0x%02X\n", i);
        }
    }
    printf("Scan completed.\n");



    return err;
}

uint16_t read_battery_state_of_charge() {
    uint8_t data[2] = {0}; // 2 bytes

    // Send command to request the state of charge register
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, STATE_OF_CHARGE_REG, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) {
        ESP_LOGE("Battery", "Error in I2C transmission: %s", esp_err_to_name(ret));
        return 0;
    }

    // Request 2 bytes from the device
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (I2C_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, 2, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) {
        ESP_LOGE("Battery", "Error in I2C read: %s", esp_err_to_name(ret));
        return 0;
    }

    uint16_t charge = (data[1] << 8) | data[0];
    return charge;
}
