#include <esp_spiffs.h>
#include <esp_wifi.h>
#include <esp_websocket_client.h>

#include "include/AP.h"
#include "include/DNS.h"
#include "include/I2C.h"
#include "include/WS.h"

#define WEBSOCKET_URI "ws://192.168.4.1/ws" // Replace with the WebSocket server URI

// global variables
httpd_handle_t server = NULL;
int client_sockets[CONFIG_MAX_CLIENTS];
char received_data[256];
SemaphoreHandle_t data_mutex;

void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;

    switch (event_id) {
        case WEBSOCKET_EVENT_ERROR:
            ESP_LOGE("battery-portal", "WebSocket error occurred");
            break;

        case WEBSOCKET_EVENT_BEFORE_CONNECT:
            ESP_LOGI("battery-portal", "WebSocket connecting...");
            break;

        case WEBSOCKET_EVENT_CONNECTED:
            ESP_LOGI("battery-portal", "WebSocket connected");
            break;

        case WEBSOCKET_EVENT_BEGIN:
            ESP_LOGI("battery-portal", "Websocket beginning...");
            break;

        case WEBSOCKET_EVENT_DATA:
            ESP_LOGI("RECEIVING", "WebSocket data: %.*s", data->data_len, (char *)data->data_ptr);
            if (xSemaphoreTake(data_mutex, portMAX_DELAY)) {
                strncpy(received_data, data->data_ptr, data->data_len);
                received_data[data->data_len] = '\0';  // Null-terminate the string
                xSemaphoreGive(data_mutex);
            } else {
                ESP_LOGE("WEBSOCKET", "Failed to take mutex for received data");
            }
            break;

        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGW("battery-portal", "WebSocket disconnected");
            break;

        default:
            ESP_LOGW("battery-portal", "Unhandled WebSocket event ID: %ld", event_id);
            break;
    }
}

void websocket_client_task(void *pvParameters) {
    QueueHandle_t *queue = (QueueHandle_t *)pvParameters;

    // Configure WebSocket client
    esp_websocket_client_config_t websocket_cfg = {
        .uri = WEBSOCKET_URI,
        .port = 80,
        .network_timeout_ms = 5000,
        .reconnect_timeout_ms = 5000,
    };

    // Initialize WebSocket client
    esp_websocket_client_handle_t client = esp_websocket_client_init(&websocket_cfg);
    esp_websocket_register_events(client, ESP_EVENT_ANY_ID, websocket_event_handler, queue);

    // Start WebSocket client
    esp_websocket_client_start(client);

    // Keep the task running indefinitely
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Add a delay to avoid task starvation
    }

    // Cleanup (this point will not be reached in the current loop)
    esp_websocket_client_stop(client);
    esp_websocket_client_destroy(client);
    vTaskDelete(NULL);
}

void app_main(void) {
    // initialise SPIFFS
    esp_vfs_spiffs_conf_t config = {
        .base_path = "/storage",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };
    esp_err_t result = esp_vfs_spiffs_register(&config);
    if (result != ESP_OK) {
        ESP_LOGE("main", "Failed to initialise SPIFFS (%s)", esp_err_to_name(result));
        return;
    }

    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI("main", "I2C initialized successfully");

    // Initialize the GPIO pin as an output for LED toggling
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // initialise mutex
    data_mutex = xSemaphoreCreateMutex();
    if (data_mutex == NULL) {
        ESP_LOGE("MAIN", "Failed to create data mutex");
        return;
    }

    // Start the Access Point and Connection
    wifi_init();
    vTaskDelay(pdMS_TO_TICKS(3000));

    // Start the ws server to serve the HTML page
    // static httpd_handle_t server = NULL;
    server = start_webserver();
    if (server == NULL) {
        ESP_LOGE("main", "Failed to start web server!");
        return;
    }

    // Start DNS server task
    xTaskCreate(&dns_server_task, "dns_server_task", 4096, NULL, 5, NULL);

    xTaskCreate(&websocket_broadcast_task, "websocket_broadcast_task", 4096, &server, 5, NULL);

    xTaskCreate(websocket_client_task, "websocket_client_task", 4096, NULL, 5, NULL);
}
