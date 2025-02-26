#ifndef CONFIG_H
#define CONFIG_H


#define AP_WIFI_SSID "AceOn battery"
#define AP_WIFI_PASS "password"
#define AP_MAX_STA_CONN 4


#define DNS_PORT 53


#define I2C_ADDR           CONFIG_I2C_ADDR           // 7-bit I2C address of the battery
#define I2C_MASTER_SDA_IO  CONFIG_I2C_MASTER_SDA_PIN // GPIO number for I2C master data
#define I2C_MASTER_SCL_IO  CONFIG_I2C_MASTER_SCL_PIN // GPIO number for I2C master clock
#define I2C_MASTER_FREQ_HZ CONFIG_I2C_FREQ_HZ        // I2C master clock frequency
#define I2C_MASTER_NUM     I2C_NUM_0                 // I2C port number for master dev

#define I2C_STATE_OF_CHARGE_REG CONFIG_STATE_OF_CHARGE_REG // Register address for StateOfCharge
#define I2C_VOLTAGE_REG CONFIG_VOLTAGE_REG                 // Register address for Voltage
#define I2C_CURRENT_REG CONFIG_CURRENT_REG                 // Register address for AverageCurrent
#define I2C_TEMPERATURE_REG CONFIG_TEMPERATURE_REG         // Register address for Temperature

#define I2C_CONTROL_REG             0x00  // Control register address
#define I2C_CONTROL_RESET_SUBCMD    0x0041 // Reset subcommand
#define I2C_I2C_MASTER_PORT         0      // I2C port number (adjust as needed)

#define I2C_DATA_FLASH_CLASS       0x3E
#define I2C_DATA_FLASH_BLOCK       0x3F
#define I2C_BLOCK_DATA_START       0x40
#define I2C_BLOCK_DATA_CHECKSUM    0x60
#define I2C_BLOCK_DATA_CONTROL     0x61

#define I2C_MASTER_TIMEOUT_MS     1000 // time delay to allow for BMS response
#define I2C_MASTER_TX_BUF_DISABLE 0    // I2C master doesn't need buffer
#define I2C_MASTER_RX_BUF_DISABLE 0    // I2C master doesn't need buffer

#define I2C_CHARGE_INHIBIT_CFG_SUBCLASS_ID 32
#define I2C_CHG_INHIBIT_TEMP_LOW_OFFSET      0
#define I2C_CHG_INHIBIT_TEMP_HIGH_OFFSET     2

#define I2C_DISCHARGE_SUBCLASS_ID 49
#define I2C_BL_OFFSET                9
#define I2C_BH_OFFSET               14

#define I2C_CURRENT_THRESHOLDS_SUBCLASS_ID 81
#define I2C_DSG_CURRENT_THRESHOLD_OFFSET     0
#define I2C_CHG_CURRENT_THRESHOLD_OFFSET     2

#define I2C_LED_GPIO_PIN 2


#define UTILS_KEY_LENGTH 16

#define UTILS_EDUROAM_USERNAME CONFIG_EDUROAM_USERNAME
#define UTILS_EDUROAM_PASSWORD CONFIG_EDUROAM_PASSWORD


#define WS_QUEUE_SIZE 10
#define WS_MESSAGE_MAX_LEN 1024

#define WS_MAX_HTML_SIZE 4096
#define WS_CONFIG_MAX_CLIENTS 5


#endif // CONFIG_H
