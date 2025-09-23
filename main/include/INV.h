#ifndef VIN_H
#define VIN_H

#include <stdint.h>

void get_display_data(uint8_t* data);

void inverter_task(void *pvParameters);

#endif
