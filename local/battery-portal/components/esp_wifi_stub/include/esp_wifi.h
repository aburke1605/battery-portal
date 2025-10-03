#pragma once

#include "esp_err.h"
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {} wifi_init_config_t;

#define WIFI_INIT_CONFIG_DEFAULT() {}

static inline esp_err_t esp_wifi_init(const wifi_init_config_t *config) {
    ESP_LOGI("[esp_wifi_stub]", "esp_wifi_init called");
    return ESP_OK;
}

#ifdef __cplusplus
}
#endif
