#include "include/AP.h"
#include "include/BMS.h"
#include "include/DNS.h"
#include "include/GPS.h"
#include "include/I2C.h"
#include "include/MESH.h"
#include "include/WS.h"
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
bool connected_to_root = false;
client_socket client_sockets[WS_CONFIG_MAX_CLIENTS];
char current_auth_token[UTILS_AUTH_TOKEN_LENGTH] = "";
QueueHandle_t ws_queue;
LoRa_message all_messages[MESH_SIZE] = {0};
char forwarded_message[LORA_MAX_PACKET_LEN-2] = "";

TaskHandle_t websocket_task_handle = NULL;
TaskHandle_t mesh_websocket_task_handle = NULL;
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

    TaskParams dns_server_params = {.stack_size = 2600, .task_name = "dns_server_task"};
    xTaskCreate(&dns_server_task, dns_server_params.task_name, dns_server_params.stack_size, &dns_server_params, 2, NULL);

    ws_queue = xQueueCreate(WS_QUEUE_SIZE, sizeof(char*));
    TaskParams message_queue_params = {.stack_size = 3900, .task_name = "message_queue_task"};
    xTaskCreate(&message_queue_task, message_queue_params.task_name, message_queue_params.stack_size, &message_queue_params, 5, NULL);

    TaskParams websocket_params = {.stack_size = 4600, .task_name = "websocket_task"};
    xTaskCreate(&websocket_task, websocket_params.task_name, websocket_params.stack_size, &websocket_params, 1, &websocket_task_handle);


    if (!LORA_IS_RECEIVER) {
        // MESH stuff
        if (!is_root) {
            TaskParams connect_to_root_params = {.stack_size = 2500, .task_name = "connect_to_root_task"};
            xTaskCreate(&connect_to_root_task, connect_to_root_params.task_name, connect_to_root_params.stack_size, &connect_to_root_params, 4, NULL);
            TaskParams mesh_websocket_params = {.stack_size = 3100, .task_name = "mesh_websocket_task"};
            xTaskCreate(&mesh_websocket_task, mesh_websocket_params.task_name, mesh_websocket_params.stack_size, &mesh_websocket_params, 3, &mesh_websocket_task_handle);
/*
        } else {
            TaskParams merge_root_params = {.stack_size = 2600, .task_name = "merge_root_task"};
            xTaskCreate(&merge_root_task, merge_root_params.task_name, merge_root_params.stack_size, &merge_root_params, 4, &merge_root_task_handle);
*/
        }
    }
}
