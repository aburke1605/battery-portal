#ifndef SPI_H
#define SPI_H

#include "include/config.h"

#include <esp_err.h>

void spi_reset();

uint8_t spi_read_register(uint8_t reg);

void spi_write_register(uint8_t reg, uint8_t value);

esp_err_t spi_init();

#endif
