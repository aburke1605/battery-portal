#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_http_server.h>
#include <esp_http_client.h>

#include "include/global.h"

void send_fake_post_request();

esp_err_t get_POST_data(httpd_req_t *req, char* content, size_t content_size);

void url_decode(char *dest, const char *src);

void url_encode(char *dest, const char *src, size_t dest_size);

char* read_file(const char* path);

char* replace_placeholder(const char *html, const char *const placeholders[], const char*const substitutes[], size_t num_replacements);

uint8_t get_block(uint8_t offset);

void check_bytes(TaskParams *params);
