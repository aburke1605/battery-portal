#pragma once

#include "esp_err.h"
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline esp_err_t esp_netif_init(void) {
    ESP_LOGI("[esp_netif_stub]", "esp_netif_init called");
    return ESP_OK;
}

static inline esp_err_t esp_netif_set_ip_info(esp_netif_t *esp_netif, const esp_netif_ip_info_t *ip_info) {
    ESP_LOGI("[esp_netif_stub]", "esp_netif_set_ip_info called");
    return ESP_OK;
}

static inline esp_err_t esp_netif_dhcps_start(esp_netif_t *esp_netif) {
    ESP_LOGI("[esp_netif_stub]", "esp_netif_dhcps_start called");
    return ESP_OK;
}

static inline esp_err_t esp_netif_dhcps_stop(esp_netif_t *esp_netif) {
    ESP_LOGI("[esp_netif_stub]", "esp_netif_dhcps_stop called");
    return ESP_OK;
}

#ifdef __cplusplus
}
#endif
