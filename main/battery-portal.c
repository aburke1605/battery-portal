#include <esp_http_client.h>
#include <esp_spiffs.h>

#include "include/AP.h"
#include "include/DNS.h"
#include "include/I2C.h"
#include "include/WS.h"

// global variables
httpd_handle_t server = NULL;
int client_sockets[CONFIG_MAX_CLIENTS];
char received_data[256];
SemaphoreHandle_t data_mutex;

void send_sensor_data_task(void *pvParameters) {
    while (true) {
        esp_http_client_config_t config = {
            .url = "http://192.168.137.1:5000/data", // this is the IPv4 address of the PC hotspot
        };

        esp_http_client_handle_t client = esp_http_client_init(&config);

        // Read sensor data
        uint16_t iCharge = read_2byte_data(STATE_OF_CHARGE_REG);

        uint16_t iVoltage = read_2byte_data(VOLTAGE_REG);
        float fVoltage = (float)iVoltage / 1000.0;

        uint16_t iCurrent = read_2byte_data(CURRENT_REG);
        float fCurrent = (float)iCurrent / 1000.0;
        if (fCurrent < 65.536 && fCurrent > 32.767) fCurrent = 65.536 - fCurrent;

        uint16_t iTemperature = read_2byte_data(TEMPERATURE_REG);
        float fTemperature = (float)iTemperature / 10.0 - 273.15;

        // Create JSON object with sensor data
        cJSON *json = cJSON_CreateObject();
        cJSON_AddNumberToObject(json, "charge", iCharge);
        cJSON_AddNumberToObject(json, "voltage", fVoltage);
        cJSON_AddNumberToObject(json, "current", fCurrent);
        cJSON_AddNumberToObject(json, "temperature", fTemperature);

        char *json_string = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);

        // Configure HTTP client for POST
        esp_http_client_set_method(client, HTTP_METHOD_POST);
        esp_http_client_set_header(client, "Content-Type", "application/json");
        esp_http_client_set_post_field(client, json_string, strlen(json_string));

        // Perform HTTP POST request
        esp_err_t err = esp_http_client_perform(client);
        if (err == ESP_OK) {
            int status_code = esp_http_client_get_status_code(client);
            char response_buf[200];
            int content_length = esp_http_client_read_response(client, response_buf, sizeof(response_buf) - 1);
            if (content_length >= 0) {
                response_buf[content_length] = '\0';  // Null-terminate the response
                printf("Sent data: %s\n", json_string);
                // ESP_LOGI("main", "HTTP POST Status = %d, Response = %s", status_code, response_buf);
            } else {
                ESP_LOGE("main", "Failed to read response");
            }
        } else {
            ESP_LOGE("main", "HTTP POST request failed: %s", esp_err_to_name(err));
        }

        esp_http_client_cleanup(client);
        free(json_string);

        vTaskDelay(1000 / portTICK_PERIOD_MS);  // Wait 1 second
    }
}

void app_main(void) {
    /*
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
    */

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
        ESP_LOGE("main", "Failed to create data mutex");
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
    xTaskCreate(&send_sensor_data_task, "send_sensor_data_task", 8192, NULL, 5, NULL);
}
