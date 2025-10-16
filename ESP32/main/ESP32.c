#include "AP.h"
#include "BMS.h"
#include "DNS.h"
#include "GPS.h"
#include "I2C.h"
#include "INV.h"
#include "LoRa.h"
#include "MESH.h"
#include "TASK.h"
#include "WS.h"
#include "config.h"
#include "utils.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif_types.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"

esp_netif_t *ap_netif;
bool is_root = false;
int num_connected_clients = 0;
uint8_t ESP_ID = 0;
httpd_handle_t server = NULL;
bool connected_to_WiFi = false;
bool connected_to_root = false;
client_socket client_sockets[WS_CONFIG_MAX_CLIENTS];
char current_auth_token[UTILS_AUTH_TOKEN_LENGTH] = "";
bool LoRa_configured = false;
LoRa_message all_messages[MESH_SIZE] = {0};
char forwarded_message[LORA_MAX_PACKET_LEN-2] = "";

QueueHandle_t job_queue;

TaskHandle_t websocket_task_handle = NULL;

void app_main(void) {
    initialise_nvs();

    initialise_spiffs();

    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI("main", "I2C initialized successfully");
    if (SCAN_I2C) device_scan();

    uart_init();

    if (!LORA_IS_RECEIVER) {
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

    job_queue = xQueueCreate(10, sizeof(job_t));
    assert(job_queue != NULL);

    xTaskCreate(job_worker_freertos_task, "job_worker_freertos_task", 10000, NULL, 5, NULL);

    xTaskCreate(dns_server_freertos_task, "dns_server_freertos_task", 2600, NULL, 5, NULL);

    start_inverter_timed_task();

    start_websocket_timed_task();

    if (!LORA_IS_RECEIVER) {
        // MESH stuff
        if (!is_root) {
            start_connect_to_root_timed_task();

            start_mesh_websocket_timed_task();
        } else {
            start_merge_root_timed_task();
        }
    }

    // radio stuff
    if (LORA_IS_RECEIVER || is_root) {
        lora_init();

        if (LORA_IS_RECEIVER) start_receive_interrupt_task();
        TaskParams lora_params = {.stack_size = 8700, .task_name = "lora_task"};
        xTaskCreate(lora_task, lora_params.task_name, lora_params.stack_size, &lora_params, 1, NULL);
    }

    while (true) {
        if (VERBOSE) ESP_LOGI("main", "%" PRId32 " bytes available in heap", esp_get_free_heap_size());
        if (esp_get_minimum_free_heap_size() < 2000) esp_restart();
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
