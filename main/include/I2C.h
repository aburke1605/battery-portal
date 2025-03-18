#include <esp_log.h>
#include <driver/i2c_master.h>

extern i2c_master_bus_handle_t i2c_bus;
extern i2c_master_dev_handle_t i2c_device;

esp_err_t i2c_master_init(void);

void device_scan(void);

esp_err_t read_data(uint8_t reg, uint8_t* data, size_t len);

uint16_t read_2byte_data(uint8_t reg);

uint16_t test_read(uint8_t subclass, uint8_t offset);

void read_name(uint8_t subclass, uint8_t offset, char* name);

esp_err_t write_byte(uint8_t reg, uint8_t data);

esp_err_t set_device_name(uint8_t subclass, uint8_t offset, char value[11]);

esp_err_t set_I2_value(uint8_t subclass, uint8_t offset, int16_t value);

esp_err_t write_word(uint8_t reg, uint16_t value);

esp_err_t reset_BMS();
