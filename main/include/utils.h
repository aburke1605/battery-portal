#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_http_server.h>
#include <esp_http_client.h>

extern bool connected_to_WiFi;

#define KEY_LENGTH 16
void random_key(char *key);

void send_fake_post_request();

esp_err_t get_POST_data(httpd_req_t *req, char* content, size_t content_size);

void url_decode(char *dest, const char *src);

void url_encode(char *dest, const char *src, size_t dest_size);

uint8_t get_block(uint8_t offset);
