#ifndef GPS_H
#define GPS_H

#include <stdbool.h>

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
} GPRMC_t;

void uart_init();

bool validate_nmea_checksum(const char *sentence);

double nmea_to_decimal(double coord, char hemi);

void parse_gprmc(char* gprmc);

void update_gps();

void start_read_gps_timed_task() ;

#endif