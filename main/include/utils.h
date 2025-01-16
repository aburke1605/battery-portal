#include <esp_log.h>
#include <esp_http_server.h>

esp_err_t get_POST_data(httpd_req_t *req, char* content, size_t content_size);

void url_decode(char *dest, const char *src);

void ping_target(const char *target_ip, uint8_t *ip_tracker);

void get_devices();

uint8_t get_block(uint8_t offset);
