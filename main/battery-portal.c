#include <string.h>

#include <esp_spiffs.h>
#include <esp_log.h>
#include <esp_http_server.h>

#include "include/AP.h"
#include "include/DNS.h"
#include "include/I2C.h"
#include "include/WS.h"

// Define the server globally
httpd_handle_t server = NULL;

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
        ESP_LOGE("SPIFFS", "Failed to initialise SPIFFS (%s)", esp_err_to_name(result));
        return;
    }

    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI("main", "I2C initialized successfully");

    uint16_t iCharge = read_2byte_data(STATE_OF_CHARGE_REG);
    ESP_LOGI("main", "Charge: %d %%", iCharge);

    uint16_t iVoltage = read_2byte_data(VOLTAGE_REG);
    float fVoltage = (float)iVoltage / 1000.0;
    ESP_LOGI("main", "Voltage: %.2f V", fVoltage);

    uint16_t iCurrent = read_2byte_data(CURRENT_REG);
    float fCurrent = (float)iCurrent / 1000.0;
    ESP_LOGI("main", "Current: %.2f A", fCurrent);

    uint16_t iTemperature = read_2byte_data(TEMPERATURE_REG);
    float fTemperature = (float)iTemperature / 10.0 - 273.15;
    ESP_LOGI("main", "Temperature: %.2f \u00B0C", fTemperature);

    // Start the Access Point
    wifi_init_softap();

    // Start the ws server to serve the HTML page
    // static httpd_handle_t server = NULL;
    server = start_webserver();
    if (server == NULL) {
        ESP_LOGE("MAIN", "Failed to start web server!");
        return;
    }

    // Start DNS server task
    xTaskCreate(&dns_server_task, "dns_server_task", 4096, NULL, 5, NULL);

    // xTaskCreate(&websocket_send_task, "websocket_send_task", 4096, server, 5, NULL);    // Create a task to periodically send WebSocket updates
    xTaskCreate(&websocket_broadcast_task, "websocket_broadcast_task", 4096, &server, 5, NULL);
}
