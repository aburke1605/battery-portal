#ifndef GLOBAL_H
#define GLOBAL_H

#include "include/config.h"

#include <stdint.h>
#include <stdbool.h>
#include "esp_netif_types.h"
#include "freertos/FreeRTOS.h"
#include "esp_http_server.h"

extern esp_netif_t *ap_netif;
extern bool is_root;
extern int num_connected_clients;
extern uint8_t ESP_ID;
extern httpd_handle_t server;
extern bool connected_to_WiFi;
extern client_socket client_sockets[WS_CONFIG_MAX_CLIENTS];
extern char current_auth_token[UTILS_AUTH_TOKEN_LENGTH];
extern QueueHandle_t ws_queue;
extern LoRa_message all_messages[MESH_SIZE];

extern TaskHandle_t websocket_task_handle;
extern TaskHandle_t merge_root_task_handle;

#endif // GLOBAL_H
