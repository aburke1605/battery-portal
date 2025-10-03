#pragma once

#include "esp_err.h"
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TAG "[esp_netif_stub]"

static inline esp_err_t esp_netif_init(void) {
    ESP_LOGI(TAG, "esp_netif_init called\n");
    return ESP_OK;
}

#ifdef __cplusplus
}
#endif
