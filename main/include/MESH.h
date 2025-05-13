#ifndef MESH_H
#define MESH_H

#include "include/config.h"

void connect_to_root_task(void *pvParameters);

void mesh_websocket_task(void *pvParameters);

esp_err_t ap_n_client_comparison_handler(esp_http_client_event_t *evt);

#endif // MESH_H
