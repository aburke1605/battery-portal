#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_websocket_client.h"
#include "esp_log.h"

#define WIFI_SSID "AceOn battery" // Replace with the SSID of the first ESP32
#define WIFI_PASS "password" // Replace with the password of the first ESP32

#define WEBSOCKET_URI "ws://192.168.4.1:80/ws" // Replace with the WebSocket server URI
static const char *TAG = "WebSocketClient";

void wifi_init(void) {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();
}

// WebSocket Event Handler
static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;

    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            ESP_LOGI(TAG, "WebSocket connected");
            break;

        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "WebSocket disconnected");
            break;

        case WEBSOCKET_EVENT_DATA:
            ESP_LOGI(TAG, "WebSocket data received: %.*s", data->data_len, (char *)data->data_ptr);
            break;

        case WEBSOCKET_EVENT_ERROR:
            ESP_LOGE(TAG, "WebSocket error occurred");
            break;

        default:
            // ESP_LOGW(TAG, "Unhandled WebSocket event ID: %d", event_id);
            break;
    }
}

void websocket_task(void *pvParameters) {
    // Configure WebSocket client
    esp_websocket_client_config_t websocket_cfg = {
        .uri = WEBSOCKET_URI,
    };

    // Initialize WebSocket client
    esp_websocket_client_handle_t client = esp_websocket_client_init(&websocket_cfg);
    esp_websocket_register_events(client, ESP_EVENT_ANY_ID, websocket_event_handler, NULL);

    // Start WebSocket client
    esp_websocket_client_start(client);

    // Keep the task running indefinitely
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Add a delay to avoid task starvation
    }

    // Cleanup (this point will not be reached in the current loop)
    esp_websocket_client_stop(client);
    esp_websocket_client_destroy(client);
    vTaskDelete(NULL);
}

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    wifi_init();

    xTaskCreate(websocket_task, "websocket_task", 4096, NULL, 5, NULL);
}