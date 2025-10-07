#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_event_base.h"
#include "esp_interface.h"

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
    WIFI_IF_AP  = ESP_IF_WIFI_AP,
} wifi_interface_t;

typedef enum {
    WIFI_AUTH_OPEN = 0,
} wifi_auth_mode_t;

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
    int8_t  rssi;
} wifi_ap_record_t;

typedef struct {
    uint8_t ssid[32];
    uint8_t ssid_len;
    uint8_t channel;
    wifi_auth_mode_t authmode;
    uint8_t max_connection;
} wifi_ap_config_t;
typedef struct {
    uint8_t ssid[32];
    uint8_t password[64];
} wifi_sta_config_t;
typedef union {
    wifi_ap_config_t  ap;
    wifi_sta_config_t sta;
} wifi_config_t;

typedef enum {
    WIFI_EVENT_STA_START,
    WIFI_EVENT_AP_STACONNECTED,
    WIFI_EVENT_AP_STADISCONNECTED,
} wifi_event_t;

ESP_EVENT_DECLARE_BASE(WIFI_EVENT);

typedef struct {
    uint8_t mac[6];
    uint8_t aid;
} wifi_event_ap_staconnected_t;
typedef struct {
    uint8_t mac[6];
    uint8_t aid;
} wifi_event_ap_stadisconnected_t;

#ifdef __cplusplus
}
#endif
