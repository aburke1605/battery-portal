#include "include/GPS.h"

#include "include/config.h"

#include <string.h>

#include "driver/uart.h"
#include "esp_log.h"

static const char* TAG = "GPS";

void uart_init() {
    const uart_config_t uart_config = {
        .baud_rate = 9600, // typical for NEO-6M
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    ESP_ERROR_CHECK(uart_param_config(GPS_UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(GPS_UART_NUM, GPS_GPIO_NUM_32, GPS_GPIO_NUM_33, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE)); // TX, RX
    ESP_ERROR_CHECK(uart_driver_install(GPS_UART_NUM, GPS_BUFF_SIZE, 0, 0, NULL, 0));
}

bool validate_nmea_checksum(const char *sentence) {
    if (sentence[0] != '$') return false;

    const char *asterisk = strchr(sentence, '*');
    if (!asterisk || (asterisk - sentence) < 1) return false;

    uint8_t checksum = 0;
    for (const char *p = sentence + 1; p < asterisk; p++) {
        checksum ^= (uint8_t)(*p);
    }

    uint8_t received = (uint8_t)strtol(asterisk + 1, NULL, 16);

    return checksum == received;
}

double nmea_to_decimal(double coord, char hemi) {
    int degrees = (int)(coord / 100);
    double minutes = coord - (degrees * 100);
    double decimal = degrees + minutes / 60.0;
    if (hemi == 'S' || hemi == 'W') {
        decimal = -decimal;
    }
    return decimal;
}

GPRMC* parse_gprmc(char* gprmc) {
    // initialise empty array
    const size_t n_fields = 12;
    const size_t max_str_len = 10;
    char data[n_fields][max_str_len];
    for (size_t i=0; i<n_fields; i++) {
        for (size_t j=0; j<max_str_len-1; j++)
            data[i][j] = 0;
        data[i][max_str_len] = '\0';
    }

    // copy non-empty strings into array
    int i = 0;
    char *start = gprmc;
    char *end = gprmc;
    while (*end != '\0' && i < n_fields) {
        if (*end == ',' || *end == '*') {
            size_t len = end - start;
            if (len >= max_str_len) len = max_str_len - 1;
            strncpy(data[i], start, len);
            data[i][len] = '\0';
            i++;
            start = end+1; // move pointer past ',' or '*'
        }
        end++;
    }
    if (strcmp(data[0], "V") == 0) {
        ESP_LOGE(TAG, "No fix: GPS data void!");
        return NULL;
    }

    GPRMC* sentence = malloc(sizeof(GPRMC));
    sentence->time = atof(data[0]);
    sentence->status = *data[1];
    sentence->latitude = nmea_to_decimal(atof(data[2]), *data[3]);
    sentence->lat_dir = *data[3];
    sentence->longitude = nmea_to_decimal(atof(data[4]), *data[5]);
    sentence->long_dir = *data[5];
    sentence->speed = atof(data[6]);
    sentence->course = atof(data[7]);
    sentence->date = atoi(data[8]);
    sentence->magnetic_variation = atoi(data[9]);
    sentence->magnetic_variation_dircetion = atoi(data[10]);
    sentence->mode = *data[11];
    if (sentence->mode != 'A') {
        ESP_LOGE(TAG, "Mode not autonomous!");
        return NULL;
    }
    return sentence;
}

GPRMC* get_gps() {
    uint8_t buff[GPS_BUFF_SIZE];
    int len = uart_read_bytes(GPS_UART_NUM, buff, GPS_BUFF_SIZE - 1, pdMS_TO_TICKS(1000));
    if (len > 0) {
        buff[len] = '\0';
        for (int i=0; i<len; i++) {
            if (buff[i] == '$') {
                for (int j=i+1; j<len; j++) {
                    if (buff[j] == '\r') {
                        int length = j - i;
                        char line[length+1];
                        strncpy(line, &((char*)buff)[i], length);
                        line[length] = '\0';

                        if (!validate_nmea_checksum(line)) {
                            ESP_LOGE(TAG, "Error in checksum!");
                            return NULL;
                        }

                        char type[6];
                        strncpy(type, &line[1], sizeof(type) - 1);
                        type[sizeof(type) - 1] = '\0';

                        if (strcmp(type, "GPRMC") == 0) {
                            int data_length = length - (sizeof(type) + 1); // +1 to account for comma
                            char data[data_length + 1];
                            // skip past the first comma
                            strncpy(data, &line[sizeof(type)+1], data_length);
                            data[data_length] = '\0';

                            GPRMC* sentence = parse_gprmc((char*)data);
                            return sentence;
                        }
                        i = j;
                        j = len; // skip to end of loop
                    }
                }
            }
        }
    }

    if (VERBOSE) ESP_LOGW(TAG, "No GPS lock");

    return NULL;
}
