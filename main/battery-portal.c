#include <esp_spiffs.h>
#include <esp_wifi.h>
#include <esp_websocket_client.h>

#include "include/AP.h"
#include "include/DNS.h"
#include "include/I2C.h"
#include "include/WS.h"

#define WEBSOCKET_URI "ws://192.168.4.1:80/ws" // Replace with the WebSocket server URI

// Define the server globally
httpd_handle_t server = NULL;
// and the remote queue
QueueHandle_t broadcast_queue;
// Shared JSON objec
cJSON *global_json = NULL;

void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    QueueHandle_t *queue = (QueueHandle_t *)handler_args;
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;

    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            ESP_LOGI("battery-portal", "WebSocket connected");
            break;

        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGI("battery-portal", "WebSocket disconnected");
            break;

        case WEBSOCKET_EVENT_DATA:
            char *received_data = strndup((char *)data->data_ptr, data->data_len);
            if (xQueueSend(*queue, &received_data, pdMS_TO_TICKS(100)) != pdPASS) {
                ESP_LOGW("WS", "Failed to enqueue received data");
                free(received_data); // Free if the queue is full
            }
            ESP_LOGI("battery-portal", "WebSocket data received: %.*s", data->data_len, (char *)data->data_ptr);


            ESP_LOGI("battery-portal", "WebSocket data received: %.*s", data->data_len, (char *)data->data_ptr);

            // Parse received JSON data
            cJSON *received_data_json = cJSON_ParseWithLength((char *)data->data_ptr, data->data_len);
            if (!received_data_json) {
                ESP_LOGE("battery-portal", "Failed to parse received WebSocket data");
                return;
            }
            // Update global JSON
            if (!global_json) {
                global_json = cJSON_CreateObject();
            }
            // Replace or merge remote_data field
            cJSON_DeleteItemFromObject(global_json, "remote_data");
            cJSON_AddItemToObject(global_json, "remote_data", received_data_json); // Ownership is transferred

            break;

        case WEBSOCKET_EVENT_ERROR:
            ESP_LOGE("battery-portal", "WebSocket error occurred");
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
    // Initialize the queue
    broadcast_queue = xQueueCreate(10, sizeof(char *));

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

    // Start WebSocket Client to connect to another ESP32
    xTaskCreate(websocket_client_task, "websocket_client_task", 4096, &broadcast_queue, 5, NULL);
}
