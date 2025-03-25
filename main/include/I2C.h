#ifndef I2C_H
#define I2C_H

#include <esp_err.h>

esp_err_t i2c_master_init(void);

esp_err_t check_device();

void device_scan(void);

esp_err_t read_data(uint8_t reg, uint8_t* data, size_t n_bytes);

esp_err_t write_data(uint8_t reg, uint32_t data, size_t n_bytes);

void read_bytes(uint8_t subclass, uint8_t offset, uint8_t* data, size_t n_bytes);

esp_err_t write_bytes(uint8_t subclass, uint8_t offset, uint8_t* data, size_t n_bytes);

#endif // I2C_H
