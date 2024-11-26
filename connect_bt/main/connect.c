#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "nvs_flash.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#include <string.h> // Added to fix implicit declaration of 'strlen'

#define GATTS_TAG "GATTS_DEMO"

// Define the name of the Bluetooth Peripheral Device
#define DEVICE_NAME "ESP32_BLE_SERVER"
#define GATTS_SERVICE_UUID_TEST   0x00FF
#define GATTS_CHAR_UUID_TEST      0xFF01
#define GATTS_NUM_HANDLE_TEST     4

// Global Variables
static uint16_t gatts_if_handle = 0xFFFF; // To store GATT interface
static uint16_t connection_id = 0; // To store connection ID
static uint16_t char_handle = 0; // To store characteristic handle
static bool is_connected = false; // Flag to indicate connection status

// Service ID for creating the GATT service
static esp_gatt_srvc_id_t gatts_service_id;

// Advertising parameters need to be globally accessible to restart advertising on disconnect
esp_ble_adv_params_t adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

// GAP event handler
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch (event) {
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(GATTS_TAG, "Advertising start failed");
            } else {
                ESP_LOGI(GATTS_TAG, "Advertising started successfully");
            }
            break;
        default:
            break;
    }
}

// GATT server event handler
static void gatts_profile_event_handler(esp_gatts_cb_event_t event, 
                                        esp_gatt_if_t gatts_if, 
                                        esp_ble_gatts_cb_param_t *param) {
    switch (event) {
        case ESP_GATTS_REG_EVT: {
            ESP_LOGI(GATTS_TAG, "REGISTER_APP_EVT, status %d, app_id %d", param->reg.status, param->reg.app_id);
            gatts_if_handle = gatts_if; // Store GATT interface

            // Initialize the service ID
            gatts_service_id.is_primary = true;
            gatts_service_id.id.inst_id = 0x00;
            gatts_service_id.id.uuid.len = ESP_UUID_LEN_16;
            gatts_service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_TEST;

            // Create GATT service
            esp_ble_gatts_create_service(gatts_if, &gatts_service_id, GATTS_NUM_HANDLE_TEST);
            break;
        }
        case ESP_GATTS_CREATE_EVT: {
            ESP_LOGI(GATTS_TAG, "CREATE_SERVICE_EVT, status %d, service_handle %d", param->create.status, param->create.service_handle);
            esp_ble_gatts_start_service(param->create.service_handle);

            // Add characteristic to the service
            esp_bt_uuid_t gatts_char_uuid;
            gatts_char_uuid.len = ESP_UUID_LEN_16;
            gatts_char_uuid.uuid.uuid16 = GATTS_CHAR_UUID_TEST;

            esp_ble_gatts_add_char(param->create.service_handle, &gatts_char_uuid,
                                   ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                   ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY,
                                   NULL, NULL);
            break;
        }
        case ESP_GATTS_ADD_CHAR_EVT: {
            ESP_LOGI(GATTS_TAG, "ADD_CHAR_EVT, status %d, attr_handle %d, service_handle %d",
                     param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
            char_handle = param->add_char.attr_handle; // Store characteristic handle
            break;
        }
        case ESP_GATTS_CONNECT_EVT: {
            ESP_LOGI(GATTS_TAG, "Device connected, connection ID: %d, Remote Address: %02x:%02x:%02x:%02x:%02x:%02x",
                     param->connect.conn_id,
                     param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
                     param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
            connection_id = param->connect.conn_id; // Store connection ID
            is_connected = true; // Set connection flag
            break;
        }
        case ESP_GATTS_DISCONNECT_EVT: {
            ESP_LOGI(GATTS_TAG, "Device disconnected, restarting advertising");
            esp_ble_gap_start_advertising(&adv_params); // Restart advertising after disconnect
            is_connected = false; // Reset connection flag
            break;
        }
        default:
            break;
    }
}

// Task to periodically send notifications every 5 seconds
void notification_task(void *arg) {
    while (true) {
        if (is_connected) {
            const char *notify_data = "Hello, Client!";
            esp_ble_gatts_send_indicate(gatts_if_handle, connection_id, char_handle,
                                        strlen(notify_data), (uint8_t *)notify_data, false);
            ESP_LOGI(GATTS_TAG, "Notification sent to client");
        }
        vTaskDelay(5000 / portTICK_PERIOD_MS); // Delay for 5 seconds
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

    // Initialize the Bluetooth Controller
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    ESP_ERROR_CHECK(ret);

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    ESP_ERROR_CHECK(ret);

    // Initialize Bluedroid stack
    ret = esp_bluedroid_init();
    ESP_ERROR_CHECK(ret);
    ret = esp_bluedroid_enable();
    ESP_ERROR_CHECK(ret);

    // Set the device name
    ret = esp_ble_gap_set_device_name(DEVICE_NAME);
    ESP_ERROR_CHECK(ret);

    // Set the advertisement data
    esp_ble_adv_data_t adv_data = {
        .set_scan_rsp = false,
        .include_name = true,
        .include_txpower = true,
        .min_interval = 0x20,
        .max_interval = 0x40,
        .appearance = 0x00,
        .manufacturer_len = 0,
        .p_manufacturer_data = NULL,
        .service_data_len = 0,
        .p_service_data = NULL,
        .service_uuid_len = 0,
        .p_service_uuid = NULL,
        .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
    };

    // Register GAP callback handler
    esp_ble_gap_register_callback(gap_event_handler);

    // Configure advertising data
    esp_ble_gap_config_adv_data(&adv_data);

    // Register GATT server callback handler
    esp_ble_gatts_register_callback(gatts_profile_event_handler);
    esp_ble_gatts_app_register(0);

    // Start advertising
    ret = esp_ble_gap_start_advertising(&adv_params);
    ESP_ERROR_CHECK(ret);

    // Create a task to send notifications every 5 seconds
    xTaskCreate(notification_task, "notification_task", 4096, NULL, 5, NULL);
}
