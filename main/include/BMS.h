#ifndef BMS_H
#define BMS_H

#include <stdbool.h>

#include <esp_err.h>

void reset();

void seal();

void unseal();

void full_access();

int8_t get_sealed_status();

char* get_data(bool local);

#endif // BMS_H
