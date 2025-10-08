#ifndef BMS_H
#define BMS_H

#include <stdint.h>

void reset();

void seal();

void unseal();

void full_access();

int8_t get_sealed_status();

#endif // BMS_H
