#ifndef GLOBAL_H
#define GLOBAL_H

#include <esp_http_server.h>
#include <esp_websocket_client.h>
#include <driver/i2c_master.h>

#include "include/config.h"

extern char ESP_ID[UTILS_KEY_LENGTH + 1];
extern httpd_handle_t server;
extern bool connected_to_WiFi;
extern char ESP_subnet_IP[15];
extern QueueHandle_t ws_queue;

#endif // GLOBAL_H