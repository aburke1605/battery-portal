#ifndef WS_H
#define WS_H

#include "include/config.h"

#include <cJSON.h>

#include <esp_err.h>
#include <esp_http_server.h>

void add_client(int fd);

void remove_client(int fd);

esp_err_t perform_request(cJSON *message, cJSON *response);

void send_ws_message(const char *message);

void message_queue_task(void *pvParameters);

void websocket_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

void websocket_task(void *pvParameters);

#endif // WS_H
