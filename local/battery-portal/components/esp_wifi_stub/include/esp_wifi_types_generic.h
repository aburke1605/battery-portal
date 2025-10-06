#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WIFI_MODE_NULL = 0,
    WIFI_MODE_STA,
    WIFI_MODE_AP,
    WIFI_MODE_APSTA
} wifi_mode_t;

typedef enum {
    WIFI_SCAN_TYPE_ACTIVE = 0,
    WIFI_SCAN_TYPE_PASSIVE,
} wifi_scan_type_t;
typedef struct {
    uint8_t *ssid;
    uint8_t *bssid;
    uint8_t channel;
    bool show_hidden;
    wifi_scan_type_t scan_type;
} wifi_scan_config_t;

typedef struct {
    uint8_t ssid[33];
} wifi_ap_record_t;

#ifdef __cplusplus
}
#endif
