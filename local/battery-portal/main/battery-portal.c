#include "include/AP.h"
#include "include/BMS.h"
#include "include/GPS.h"
#include "include/I2C.h"
#include "include/config.h"
#include "include/utils.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "esp_netif_types.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_http_server.h"

esp_netif_t *ap_netif;
bool is_root = false;
int num_connected_clients = 0;
uint8_t ESP_ID = 0;
httpd_handle_t server = NULL;
bool connected_to_WiFi = false;
client_socket client_sockets[WS_CONFIG_MAX_CLIENTS];
char current_auth_token[UTILS_AUTH_TOKEN_LENGTH] = "";
QueueHandle_t ws_queue;
LoRa_message all_messages[MESH_SIZE] = {0};

TaskHandle_t websocket_task_handle = NULL;
TaskHandle_t merge_root_task_handle = NULL;

void app_main(void) {
    initialise_nvs();

    initialise_spiffs();

    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI("main", "I2C initialized successfully");
    if (SCAN_I2C) device_scan();

    uart_init();

    if (!LORA_IS_RECEIVER) {
        // do a BMS reset on boot
        reset();
        while (get_sealed_status() != 1) {
            unseal();
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

        // grab BMS DeviceName from the BMS DataFlash
        uint8_t address[2] = {0};
        convert_uint_to_n_bytes(I2C_DEVICE_NAME_ADDR, address, sizeof(address), true);
        uint8_t data_flash[UTILS_ID_LENGTH + 1] = {0}; // S21 data type
        read_data_flash(address, sizeof(address), data_flash, sizeof(data_flash));
        // store the ID in ESP32 memory
        if (strcmp((char *)data_flash, "") != 0)
            change_esp_id((char*)&data_flash[1]);
    }

    wifi_init();
    vTaskDelay(pdMS_TO_TICKS(3000));

    server = start_webserver();
    if (server == NULL) {
        ESP_LOGE("main", "Failed to start web server!");
        return;
    }
}
