#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_http_server.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

void change_esp_id(char* name);

void initialise_nvs();

void initialise_spiffs();

esp_err_t get_POST_data(httpd_req_t *req, char* content, size_t content_size);

void convert_uint_to_n_bytes(unsigned int input, uint8_t *output, size_t n_bytes, bool little_endian);

void url_decode(char *dest, const char *src);

void random_token(char *key);

char* read_file(const char* path);

#endif // UTILS_H
