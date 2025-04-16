#ifndef GLOBAL_H
#define GLOBAL_H

#include "include/config.h"

#include <esp_http_server.h>

extern char ESP_ID[UTILS_ID_LENGTH + 1];
extern httpd_handle_t server;
extern bool connected_to_WiFi;
extern char ESP_subnet_IP[15];
extern client_socket client_sockets[WS_CONFIG_MAX_CLIENTS];
extern char current_auth_token[UTILS_AUTH_TOKEN_LENGTH];
extern QueueHandle_t ws_queue;

extern TaskHandle_t websocket_task_handle;

#endif // GLOBAL_H
