#pragma once

#include "esp_err.h"
#include "esp_log.h"
#include "esp_netif_types.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline esp_err_t esp_netif_init(void) {
    ESP_LOGI("[esp_netif_stub]", "esp_netif_init called");
    return ESP_OK;
}

static inline esp_err_t esp_netif_get_ip_info(esp_netif_t *esp_netif, esp_netif_ip_info_t *ip_info) {
    ESP_LOGI("[esp_netif_stub]", "esp_netif_get_ip_info called");
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

static inline char *esp_ip4addr_ntoa(const esp_ip4_addr_t *addr, char *buf, int buflen) {
    ESP_LOGI("[esp_netif_stub]", "esp_ip4addr_ntoa called");
    return "";
}

static inline esp_netif_t *esp_netif_get_handle_from_ifkey(const char *if_key) {
    ESP_LOGI("[esp_netif_stub]", "esp_netif_get_handle_from_ifkey called");
    return NULL;
}

#ifdef __cplusplus
}
#endif
