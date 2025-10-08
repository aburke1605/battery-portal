#ifndef WS_H
#define WS_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_http_server.h"
#include "cJSON.h"

void add_client(int fd, const char* tkn, bool browser, uint8_t esp_id);

void remove_client(int fd);

esp_err_t client_handler(httpd_req_t *req);

esp_err_t perform_request(cJSON *message, cJSON *response);

void send_message(const char *message);

void message_queue_task(void *pvParameters);

void websocket_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

void websocket_task(void *pvParameters);

#endif // WS_H
