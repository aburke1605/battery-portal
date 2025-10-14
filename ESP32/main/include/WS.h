#ifndef WS_H
#define WS_H

#include <stdbool.h>
#include <stdint.h>

#include "cJSON.h"
#include "esp_err.h"
#include "esp_http_server.h"

void add_client(int fd, const char* tkn, bool browser, uint8_t esp_id);

void remove_client(int fd);

esp_err_t client_handler(httpd_req_t *req);

esp_err_t perform_request(cJSON *message, cJSON *response);

void send_message(const char *message);

void websocket_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

char* convert_data_numbers_for_frontend(char* data_string);

void send_websocket_data();

void start_websocket_timed_task();

#endif // WS_H
