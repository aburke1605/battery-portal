#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct esp_websocket_client *esp_websocket_client_handle_t;

static inline int esp_websocket_client_send_text(esp_websocket_client_handle_t client, const char *data, int len, TickType_t timeout) {
    ESP_LOGI("[esp_websocket_client_stub]", "esp_websocket_client_send_text called");
    return ESP_OK;
}

static inline bool esp_websocket_client_is_connected(esp_websocket_client_handle_t client) {
    ESP_LOGI("[esp_websocket_client_stub]", "esp_websocket_client_is_connected called");
    return ESP_OK;
}

#ifdef __cplusplus
}
#endif
