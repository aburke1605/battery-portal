#ifndef I2C_H
#define I2C_H

#include "esp_err.h"

esp_err_t i2c_master_init(void);

esp_err_t check_device();

void device_scan(void);

esp_err_t read_SBS_data(uint8_t reg, uint8_t* data, size_t data_size);

#endif // I2C_H
