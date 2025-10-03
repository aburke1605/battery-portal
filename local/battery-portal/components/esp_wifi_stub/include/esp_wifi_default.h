#pragma once

#include "esp_err.h"
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline esp_err_t esp_netif_create_default_wifi_sta(void) {
    ESP_LOGI("[esp_wifi_stub]", "esp_netif_create_default_wifi_sta called");
    return ESP_OK;
}

#ifdef __cplusplus
}
#endif
