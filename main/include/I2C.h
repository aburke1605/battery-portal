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

#define I2C_MASTER_TIMEOUT_MS     1000 // time delay to allow for BMS response
#define I2C_MASTER_TX_BUF_DISABLE 0    // I2C master doesn't need buffer
#define I2C_MASTER_RX_BUF_DISABLE 0    // I2C master doesn't need buffer

#define CONFIGURATION_DISCHARGE_SUBCLASS_ID 0x31
#define FIRST_DATA_BLOCK                    0x00
#define SECOND_DATA_BLOCK                   0x01
#define THIRD_DATA_BLOCK                    0x02
#define FOURTH_DATA_BLOCK                   0x03

#define BL_OFFSET 0x09

esp_err_t i2c_master_init(void);

void device_scan(void);

uint16_t read_2byte_data(int REG_ADDR);

esp_err_t set_BL_voltage_threshold(int16_t BL);
