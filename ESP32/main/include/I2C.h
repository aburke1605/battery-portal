#ifndef I2C_H
#define I2C_H

#include "esp_err.h"

esp_err_t i2c_master_init(void);

esp_err_t check_device();

void device_scan(void);

esp_err_t read_SBS_data(uint8_t reg, uint8_t* data, size_t data_size);

void write_word(uint8_t command, uint8_t* word, size_t word_size);

void read_data_flash(uint8_t* address, size_t address_size, uint8_t* data, size_t data_size);

void write_data_flash(uint8_t* address, size_t address_size, uint8_t* data, size_t data_size);

void write_to_slave_esp32();

void read_from_slave_esp32();

#endif // I2C_H
