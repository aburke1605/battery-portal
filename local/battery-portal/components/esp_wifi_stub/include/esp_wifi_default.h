#pragma once

#include "esp_err.h"
#include "esp_log.h"
#include "esp_netif_types.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline esp_netif_t* esp_netif_create_default_wifi_ap(void) {
    ESP_LOGI("[esp_wifi_stub]", "esp_netif_create_default_wifi_ap called");
    return ESP_OK;
}

static inline esp_err_t esp_netif_create_default_wifi_sta(void) {
    ESP_LOGI("[esp_wifi_stub]", "esp_netif_create_default_wifi_sta called");
    return ESP_OK;
}

#ifdef __cplusplus
}
#endif
