#ifndef BMS_H
#define BMS_H

#include <stdbool.h>

#include <esp_err.h>

void reset();

void seal();

void unseal();

char* get_data(bool local);

#endif // BMS_H
