#ifndef INV_H
#define INV_H

#include <stdint.h>

#include "freertos/FreeRTOS.h"

void get_display_data(uint8_t* data);

void start_slave_esp32_timed_task();

#endif // INV_H
