#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "nvs_flash.h"
#include "esp_gattc_api.h"
#include "esp_gap_ble_api.h"
#include "esp_bt_main.h"
#include <string.h>

#define GATTC_TAG "GATTC_DEMO"

static esp_ble_scan_params_t ble_scan_params = {
    .scan_type = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval = 0x50,
    .scan_window = 0x30,
    .scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE
};

static esp_gatt_if_t gattc_if_global;

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch (event) {
        case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
            ESP_LOGI(GATTC_TAG, "ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: Scan parameters set, starting scan...");
            esp_ble_gap_start_scanning(30); // Start scanning for 30 seconds
            break;

        case ESP_GAP_BLE_SCAN_RESULT_EVT:
            if (param->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) {
                ESP_LOGI(GATTC_TAG, "Device found: %02x:%02x:%02x:%02x:%02x:%02x",
                         param->scan_rst.bda[0], param->scan_rst.bda[1], param->scan_rst.bda[2],
                         param->scan_rst.bda[3], param->scan_rst.bda[4], param->scan_rst.bda[5]);
                
                // Check if the device name matches "ESP32_BLE_SERVER_1"
                // char name[30];
                // int name_len = esp_ble_resolve_adv_data(param->scan_rst.ble_adv, ESP_BLE_AD_TYPE_NAME_CMPL, (uint8_t *)name);
                // if (name_len > 0 && name_len < sizeof(name)) {
                //     name[name_len] = '\0'; // Null-terminate to avoid buffer overflow
                //     ESP_LOGI(GATTC_TAG, "Found name: %s", name);
                //     if (strcmp(name, "ESP32_BLE_SERVER") == 0) {
                //         ESP_LOGI(GATTC_TAG, "Target server found, connecting...");
                //         esp_ble_gap_stop_scanning();
                //         esp_ble_gattc_open(gattc_if_global, param->scan_rst.bda, BLE_ADDR_TYPE_PUBLIC, true);
                //     }
                // }

                if (param->scan_rst.bda[0] == 0xd4 && param->scan_rst.bda[1] == 0xd4 &&
                    param->scan_rst.bda[2] == 0xda && param->scan_rst.bda[3] == 0x59 &&
                    param->scan_rst.bda[4] == 0xd1 && param->scan_rst.bda[5] == 0x32) {
                    ESP_LOGI(GATTC_TAG, "Target server found, connecting...");
                    esp_ble_gap_stop_scanning();
                    esp_ble_gattc_open(gattc_if_global, param->scan_rst.bda, BLE_ADDR_TYPE_PUBLIC, true);
                }
            }
            break;

        case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
            ESP_LOGI(GATTC_TAG, "ESP_GAP_BLE_SCAN_START_COMPLETE_EVT Event Id %d", event);
            if (param->scan_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
                ESP_LOGI(GATTC_TAG, "Scanning started successfully.");
            } else {
                ESP_LOGE(GATTC_TAG, "Failed to start scanning, error status = %d", param->scan_start_cmpl.status);
            }
            break;

        case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
            if (param->scan_stop_cmpl.status == ESP_BT_STATUS_SUCCESS) {
                ESP_LOGI(GATTC_TAG, "Scanning stopped successfully.");
            } else {
                ESP_LOGE(GATTC_TAG, "Failed to stop scanning, error status = %d", param->scan_stop_cmpl.status);
            }
            break;
        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
            ESP_LOGI(GATTC_TAG, "ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT, status %d, min_int %d, max_int %d, latency %d, timeout %d",
                    param->update_conn_params.status,
                    param->update_conn_params.min_int,
                    param->update_conn_params.max_int,
                    param->update_conn_params.latency,
                    param->update_conn_params.timeout);
            break;
        case ESP_GAP_BLE_SET_PKT_LENGTH_COMPLETE_EVT:
            ESP_LOGI(GATTC_TAG, "ESP_GAP_BLE_SET_PKT_LENGTH_COMPLETE_EVT");
            break;
        default:
            ESP_LOGI(GATTC_TAG, "Unhandled GAP event %d", event);
            break;
    }
}

static void gattc_profile_event_handler(esp_gattc_cb_event_t event,
                                        esp_gatt_if_t gattc_if,
                                        esp_ble_gattc_cb_param_t *param) {
    switch (event) {
        case ESP_GATTC_REG_EVT:
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_REG_EVT, status %d, app_id %d", param->reg.status, param->reg.app_id);
            gattc_if_global = gattc_if;
            if (param->reg.status == ESP_GATT_OK) {
                esp_ble_gap_set_scan_params(&ble_scan_params);
                ESP_LOGI(GATTC_TAG, "Starting BLE scan...");
            }
            break;

        case ESP_GATTC_CONNECT_EVT:
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_CONNECT_EVT Event Id %d", event);
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_CONNECT_EVT, conn_id %d, gatt_if %d, remote_bda %02x:%02x:%02x:%02x:%02x:%02x",
                     param->connect.conn_id, gattc_if,
                     param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
                     param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
            ESP_LOGI(GATTC_TAG, "Successfully connected to the Bluetooth server.");
            break;

        case ESP_GATTC_DISCONNECT_EVT:
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_DISCONNECT_EVT, conn_id %d, remote_bda %02x:%02x:%02x:%02x:%02x:%02x",
                     param->disconnect.conn_id,
                     param->disconnect.remote_bda[0], param->disconnect.remote_bda[1], param->disconnect.remote_bda[2],
                     param->disconnect.remote_bda[3], param->disconnect.remote_bda[4], param->disconnect.remote_bda[5]);
            ESP_LOGI(GATTC_TAG, "Disconnected from the Bluetooth server.");
            break;

        case ESP_GATTC_OPEN_EVT:
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_OPEN_EVT Event Id %d", event);
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_OPEN_EVT, status %d, conn_id %d, gatt_if %d",
                     param->open.status, param->open.conn_id, gattc_if);
            if (param->open.status == ESP_GATT_OK) {
                ESP_LOGI(GATTC_TAG, "Connection to Bluetooth server opened successfully.");
            } else {
                ESP_LOGI(GATTC_TAG, "Failed to open connection to Bluetooth server, status %d", param->open.status);
            }
            break;

        case ESP_GATTC_CLOSE_EVT:
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_CLOSE_EVT, status %d, conn_id %d",
                     param->close.status, param->close.conn_id);
            ESP_LOGI(GATTC_TAG, "Connection to Bluetooth server closed.");
            break;

        case ESP_GATTC_SEARCH_CMPL_EVT:
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_SEARCH_CMPL_EVT, conn_id %d, status %d",
                     param->search_cmpl.conn_id, param->search_cmpl.status);
            break;

        case ESP_GATTC_READ_CHAR_EVT:
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_READ_CHAR_EVT, status %d, value_len %d",
                     param->read.status, param->read.value_len);
            break;

        case ESP_GATTC_WRITE_CHAR_EVT:
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_WRITE_CHAR_EVT, status %d, handle %d",
                     param->write.status, param->write.handle);
            break;
        case ESP_GATTC_NOTIFY_EVT:
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_NOTIFY_EVT, conn_id %d, handle %d, value_len %d",
                     param->notify.conn_id, param->notify.handle, param->notify.value_len);
            ESP_LOG_BUFFER_HEX(GATTC_TAG, param->notify.value, param->notify.value_len);
            break;
        case ESP_GATTC_DIS_SRVC_CMPL_EVT:
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_DIS_SRVC_CMPL_EVT, conn_id %d, status %d",
                     param->dis_srvc_cmpl.conn_id, param->dis_srvc_cmpl.status);
            break;
        default:
            ESP_LOGI(GATTC_TAG, "Unhandled event %d", event);
            break;
    }
}

void app_main() {
    esp_err_t ret;

    // Initialize NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize Bluetooth controller
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    ESP_ERROR_CHECK(ret);

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    ESP_ERROR_CHECK(ret);

    // Initialize Bluedroid
    ret = esp_bluedroid_init();
    ESP_ERROR_CHECK(ret);
    ret = esp_bluedroid_enable();
    ESP_ERROR_CHECK(ret);

    // Register GAP callback
    esp_ble_gap_register_callback(gap_event_handler);

    // Register GATT client
    esp_ble_gattc_register_callback(gattc_profile_event_handler);
    esp_ble_gattc_app_register(0);
}
