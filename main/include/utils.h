#ifndef UTILS_H
#define UTILS_H

#include "include/config.h"

#include <esp_err.h>
#include <esp_http_server.h>

void send_fake_request();

esp_err_t get_POST_data(httpd_req_t *req, char* content, size_t content_size);

void url_decode(char *dest, const char *src);

void url_encode(char *dest, const char *src, size_t dest_size);

void random_token(char *key);

char* read_file(const char* path);

char* replace_placeholder(const char *html, const char *const placeholders[], const char*const substitutes[], size_t num_replacements);

uint8_t get_block(uint8_t offset);

#endif // UTILS_H
