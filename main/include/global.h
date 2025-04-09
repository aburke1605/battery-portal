#ifndef GLOBAL_H
#define GLOBAL_H

#include "include/config.h"

#include <esp_http_server.h>

extern char ESP_ID[UTILS_KEY_LENGTH + 1];
extern httpd_handle_t server;
extern bool connected_to_WiFi;
extern char ESP_subnet_IP[15];
extern int client_sockets[WS_CONFIG_MAX_CLIENTS];
extern QueueHandle_t ws_queue;

#endif // GLOBAL_H
