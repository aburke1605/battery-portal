#ifndef GPS_H
#define GPS_H

#include "include/config.h"

typedef struct {
    float time;
    char status;
    float latitude;
    char lat_dir;
    float longitude;
    char long_dir;
    float speed;
    float course;
    int date;
    int magnetic_variation;
    int magnetic_variation_dircetion;
    char mode;
} GPRMC;

void uart_init();

void gps_reset();

bool validate_nmea_checksum(const char *sentence);

GPRMC* parse_gprmc(char* gprmc);

GPRMC* get_gps();

#endif