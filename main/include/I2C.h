#include <esp_log.h>
#include <driver/i2c.h>

#define I2C_ADDR           CONFIG_I2C_ADDR           // 7-bit I2C address of the battery
#define I2C_MASTER_SDA_IO  CONFIG_I2C_MASTER_SDA_PIN // GPIO number for I2C master data
#define I2C_MASTER_SCL_IO  CONFIG_I2C_MASTER_SCL_PIN // GPIO number for I2C master clock
#define I2C_MASTER_FREQ_HZ CONFIG_I2C_FREQ_HZ        // I2C master clock frequency
#define I2C_MASTER_NUM     I2C_NUM_0                 // I2C port number for master dev

#define STATE_OF_CHARGE_REG CONFIG_STATE_OF_CHARGE_REG // Register address for StateOfCharge
#define VOLTAGE_REG CONFIG_VOLTAGE_REG                 // Register address for Voltage
#define CURRENT_REG CONFIG_CURRENT_REG                 // Register address for AverageCurrent
#define TEMPERATURE_REG CONFIG_TEMPERATURE_REG         // Register address for Temperature

#define DATA_FLASH_CLASS       0x3E
#define DATA_FLASH_BLOCK       0x3F
#define BLOCK_DATA_START       0x40
#define BLOCK_DATA_CHECKSUM    0x60
#define BLOCK_DATA_CONTROL     0x61

#define I2C_MASTER_TIMEOUT_MS     1000 // time delay to allow for BMS response
#define I2C_MASTER_TX_BUF_DISABLE 0    // I2C master doesn't need buffer
#define I2C_MASTER_RX_BUF_DISABLE 0    // I2C master doesn't need buffer

#define DISCHARGE_SUBCLASS_ID 49
#define BL_OFFSET               9
#define BH_OFFSET               14

#define IT_CFG_SUBCLASS_ID  80
#define TERM_V_DELTA_OFFSET   69

esp_err_t i2c_master_init(void);

void device_scan(void);

esp_err_t read_data(uint8_t reg, uint8_t* data, size_t len);

uint16_t read_2byte_data(uint8_t reg);

uint16_t test_read(uint8_t subclass, uint8_t offset);

esp_err_t write_byte(uint8_t reg, uint8_t data);

esp_err_t set_I2_value(uint8_t subclass, uint8_t offset, int16_t value);
