#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_event_base.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct esp_websocket_client *esp_websocket_client_handle_t;

typedef enum {
    WEBSOCKET_EVENT_ANY = -1,
    WEBSOCKET_EVENT_ERROR = 0,
    WEBSOCKET_EVENT_CONNECTED,
    WEBSOCKET_EVENT_DISCONNECTED,
    WEBSOCKET_EVENT_DATA,
} esp_websocket_event_id_t;

typedef struct {
    const char *data_ptr;
    int data_len;
} esp_websocket_event_data_t;

typedef struct {
    const char                  *uri;
    const char                  *cert_pem;
    bool                        skip_cert_common_name_check;
    int                         reconnect_timeout_ms;
    int                         network_timeout_ms;
} esp_websocket_client_config_t;

static inline esp_websocket_client_handle_t esp_websocket_client_init(const esp_websocket_client_config_t *config) {
    ESP_LOGI("[esp_websocket_client_stub]", "esp_websocket_client_init called");
    esp_websocket_client_handle_t a = {0};
    return a;
}

static inline esp_err_t esp_websocket_client_start(esp_websocket_client_handle_t client) {
    ESP_LOGI("[esp_websocket_client_stub]", "esp_websocket_client_start called");
    return ESP_OK;
}

static inline esp_err_t esp_websocket_client_stop(esp_websocket_client_handle_t client) {
    ESP_LOGI("[esp_websocket_client_stub]", "esp_websocket_client_stop called");
    return ESP_OK;
}

static inline esp_err_t esp_websocket_client_destroy(esp_websocket_client_handle_t client) {
    ESP_LOGI("[esp_websocket_client_stub]", "esp_websocket_client_destroy called");
    return ESP_OK;
}

static inline int esp_websocket_client_send_text(esp_websocket_client_handle_t client, const char *data, int len, TickType_t timeout) {
    ESP_LOGI("[esp_websocket_client_stub]", "esp_websocket_client_send_text called");
    return 1;
}

static inline bool esp_websocket_client_is_connected(esp_websocket_client_handle_t client) {
    ESP_LOGI("[esp_websocket_client_stub]", "esp_websocket_client_is_connected called");
    return true;
}

static inline esp_err_t esp_websocket_register_events(esp_websocket_client_handle_t client,
                                                      esp_websocket_event_id_t event,
                                                      esp_event_handler_t event_handler,
                                                      void *event_handler_arg) {
    ESP_LOGI("[esp_websocket_client_stub]", "esp_websocket_register_events called");
    return ESP_OK;
}

#ifdef __cplusplus
}
#endif
