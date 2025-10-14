#ifndef MESH_H
#define MESH_H

#include "esp_err.h"
#include "esp_http_client.h"

void connect_to_root();

void start_connect_to_root_timed_task();

void send_mesh_websocket_data();

void start_mesh_websocket_timed_task();

esp_err_t ap_n_client_comparison_handler(esp_http_client_event_t *evt);

void merge_root_task(void *pvParameters);

#endif // MESH_H
