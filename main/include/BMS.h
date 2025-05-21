#ifndef BMS_H
#define BMS_H

#include <esp_err.h>

esp_err_t reset();

esp_err_t unseal();

char* get_data();

#endif // BMS_H
