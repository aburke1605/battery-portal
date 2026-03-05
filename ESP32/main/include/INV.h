#ifndef INV_H
#define INV_H

#include <stdint.h>

typedef struct {
  uint8_t status;
  uint16_t output_voltage;
  uint16_t battery_voltage;
  uint8_t temperature;
  uint16_t output_power;
  char mode;
} inverter_data_t;

void uart_inv_init();

void update_inv();

#endif
