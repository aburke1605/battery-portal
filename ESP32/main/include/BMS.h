#ifndef BMS_H
#define BMS_H

#include <stdint.h>

typedef struct {
    int Q;
    int H;
    float aT;
    float cT;
    float T1;
    float T2;
    float T3;
    float T4;
    float V;
    float V1;
    float V2;
    float V3;
    float V4;
    float I;
    float I1;
    float I2;
    float I3;
    float I4;
    float OTC;
} telemetry_data_t;

void reset();

void seal();

void unseal();

void full_access();

int8_t get_sealed_status();

void update_telemetry_data();

void start_read_data_timed_task();

#endif // BMS_H
