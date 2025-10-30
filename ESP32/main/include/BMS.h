#ifndef BMS_H
#define BMS_H

#include <stdint.h>

typedef struct {
    uint8_t Q;
    uint8_t H;
    uint16_t aT;
    uint16_t cT;
    uint16_t T1;
    uint16_t T2;
    uint16_t T3;
    uint16_t T4;
    uint16_t V;
    uint16_t V1;
    uint16_t V2;
    uint16_t V3;
    uint16_t V4;
    int16_t I;
    int16_t I1;
    int16_t I2;
    int16_t I3;
    int16_t I4;
    int16_t OTC;
} telemetry_data_t;

void reset();

void seal();

void unseal();

void full_access();

int8_t get_sealed_status();

void update_telemetry_data();

void start_read_data_timed_task();

#endif // BMS_H
