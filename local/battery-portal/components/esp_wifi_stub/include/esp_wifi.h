#pragma once

#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi_types_generic.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {} wifi_init_config_t;
#define WIFI_INIT_CONFIG_DEFAULT() {}
static inline esp_err_t esp_wifi_init(const wifi_init_config_t *config) {
    ESP_LOGI("[esp_wifi_stub]", "esp_wifi_init called");
    return ESP_OK;
}

static inline esp_err_t esp_wifi_set_mode(wifi_mode_t mode) {
    ESP_LOGI("[esp_wifi_stub]", "esp_wifi_set_mode called");
    return ESP_OK;
}

static inline esp_err_t esp_wifi_start(void) {
    ESP_LOGI("[esp_wifi_stub]", "esp_wifi_start called");
    return ESP_OK;
}

static inline esp_err_t esp_wifi_stop(void) {
    ESP_LOGI("[esp_wifi_stub]", "esp_wifi_stop called");
    return ESP_OK;
}

static inline esp_err_t esp_wifi_scan_start(const wifi_scan_config_t *config, bool block) {
    ESP_LOGI("[esp_wifi_stub]", "esp_wifi_scan_start called");
    return ESP_OK;
}

static inline esp_err_t esp_wifi_scan_get_ap_num(uint16_t *number) {
    ESP_LOGI("[esp_wifi_stub]", "esp_wifi_scan_get_ap_num called");
    return ESP_OK;
}

static inline esp_err_t esp_wifi_scan_get_ap_records(uint16_t *number, wifi_ap_record_t *ap_records) {
    ESP_LOGI("[esp_wifi_stub]", "esp_wifi_scan_get_ap_records called");
    return ESP_OK;
}

static inline esp_err_t esp_wifi_set_config(wifi_interface_t interface, wifi_config_t *conf) {//993
    ESP_LOGI("[esp_wifi_stub]", "esp_wifi_set_config called");
    return ESP_OK;
}

#ifdef __cplusplus
}
#endif
