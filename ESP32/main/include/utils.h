#ifndef UTILS_H
#define UTILS_H

#include "include/config.h"

#include <esp_err.h>
#include <esp_http_server.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

void change_esp_id(char* name);

void initialise_nvs();

void initialise_spiffs();

void send_fake_request();

char *send_fake_login_post_request();

esp_err_t get_POST_data(httpd_req_t *req, char* content, size_t content_size);

void convert_uint_to_n_bytes(unsigned int input, uint8_t *output, size_t n_bytes, bool little_endian);

void url_decode(char *dest, const char *src);

void url_encode(char *dest, const char *src, size_t dest_size);

void random_token(char *key);

int round_to_dp(float var, int ndp);

char* read_file(const char* path);

char* replace_placeholder(const char *html, const char *const placeholders[], const char*const substitutes[], size_t num_replacements);

uint8_t get_block(uint8_t offset);

int compare_mac(const uint8_t *mac1, const uint8_t *mac2);

float calculate_symbol_length(uint8_t spreading_factor, uint8_t bandwidth);

int calculate_transmission_delay(uint8_t spreading_factor, uint8_t bandwidth, uint8_t n_preamble_symbols, uint16_t payload_length, uint8_t coding_rate, bool header, bool low_data_rate_optimisation);

#endif // UTILS_H
