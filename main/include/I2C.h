#include <esp_log.h>
#include <driver/i2c.h>

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

esp_err_t i2c_master_init(void);

void device_scan(void);

esp_err_t read_data(uint8_t reg, uint8_t* data, size_t len);

uint16_t read_2byte_data(uint8_t reg);

uint16_t test_read(uint8_t subclass, uint8_t offset);

esp_err_t write_byte(uint8_t reg, uint8_t data);

esp_err_t set_I2_value(uint8_t subclass, uint8_t offset, int16_t value);

esp_err_t write_word(uint8_t reg, uint16_t value);

esp_err_t reset_BMS();
