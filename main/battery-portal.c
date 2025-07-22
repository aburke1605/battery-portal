#include "include/config.h"
#include "include/global.h"
#include "include/AP.h"
#include "include/DNS.h"
#include "include/I2C.h"
#include "include/BMS.h"
#include "include/MESH.h"
#include "include/LoRa.h"
#include "include/WS.h"
#include "include/utils.h"

#include <esp_log.h>
#include <esp_spiffs.h>
#include <driver/gpio.h>

// global variables
esp_netif_t *ap_netif;
bool is_root = false;
int num_connected_clients = 0;
char ESP_ID[UTILS_ID_LENGTH + 1] = "unknown";
httpd_handle_t server = NULL;
bool connected_to_WiFi = false;
bool connected_to_root = false;
client_socket client_sockets[WS_CONFIG_MAX_CLIENTS];
char current_auth_token[UTILS_AUTH_TOKEN_LENGTH] = "";
QueueHandle_t ws_queue;
LoRa_message all_messages[MESH_SIZE] = {0};

TaskHandle_t websocket_task_handle = NULL;

TaskHandle_t mesh_websocket_task_handle = NULL;
TaskHandle_t merge_root_task_handle = NULL;

void app_main(void) {
    // initialise SPIFFS
    esp_err_t result;

    esp_vfs_spiffs_conf_t config_static = {
        .base_path = "/static",
        .partition_label = "static",
        .max_files = 5,
        .format_if_mount_failed = true
    };
    result = esp_vfs_spiffs_register(&config_static);

    if (result != ESP_OK) {
        ESP_LOGE("main", "Failed to initialise SPIFFS (%s)", esp_err_to_name(result));
        return;
    }

    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI("main", "I2C initialized successfully");
    if (SCAN_I2C) device_scan();

    if (!LORA_IS_RECEIVER) {
        // do a BMS reset on boot
        reset();
        while (get_sealed_status() != 1) {
            unseal();
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

        // Initialize the GPIO pin as an output for LED toggling
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << I2C_LED_GPIO_PIN),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        gpio_config(&io_conf);

        uint8_t address[2] = {0};
        convert_uint_to_n_bytes(I2C_DEVICE_NAME_ADDR, address, sizeof(address), true);
        uint8_t data_flash[UTILS_ID_LENGTH + 1] = {0}; // S21 data type
        read_data_flash(address, sizeof(address), data_flash, sizeof(data_flash));
        uint8_t name_length = MIN(data_flash[0], UTILS_ID_LENGTH);
        if (strcmp((char *)data_flash, "") != 0) strncpy(ESP_ID, (char *)&data_flash[1], name_length);
        ESP_ID[name_length] = '\0';
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

    esp_log_level_set("wifi", ESP_LOG_ERROR);
    esp_log_level_set("websocket_client", ESP_LOG_WARN);
    esp_log_level_set("transport_ws", ESP_LOG_WARN);
    esp_log_level_set("transport_base", ESP_LOG_WARN);
    TaskParams websocket_params = {.stack_size = 4600, .task_name = "websocket_task"};
    xTaskCreate(&websocket_task, websocket_params.task_name, websocket_params.stack_size, &websocket_params, 1, &websocket_task_handle);


    if (LORA_IS_RECEIVER) {
        lora_init();

        TaskParams lora_rx_params = {.stack_size = 6000, .task_name = "lora_rx_task"};
        xTaskCreate(lora_rx_task, lora_rx_params.task_name, lora_rx_params.stack_size, &lora_rx_params, 1, NULL);
    }

    else {
        if (!is_root) {
            TaskParams connect_to_root_params = {.stack_size = 2500, .task_name = "connect_to_root_task"};
            xTaskCreate(&connect_to_root_task, connect_to_root_params.task_name, connect_to_root_params.stack_size, &connect_to_root_params, 4, NULL);

            TaskParams mesh_websocket_params = {.stack_size = 3100, .task_name = "mesh_websocket_task"};
            xTaskCreate(&mesh_websocket_task, mesh_websocket_params.task_name, mesh_websocket_params.stack_size, &mesh_websocket_params, 3, &mesh_websocket_task_handle);
        } else {
            TaskParams merge_root_params = {.stack_size = 2300, .task_name = "merge_root_task"};
            xTaskCreate(&merge_root_task, merge_root_params.task_name, merge_root_params.stack_size, &merge_root_params, 4, &merge_root_task_handle);

            lora_init();

            TaskParams lora_tx_params = {.stack_size = 8700, .task_name = "lora_tx_task"};
            xTaskCreate(lora_tx_task, lora_tx_params.task_name, lora_tx_params.stack_size, &lora_tx_params, 3, NULL);
        }
    }

    while (true) {
        if (VERBOSE) ESP_LOGI("main", "%ld bytes available in heap", esp_get_free_heap_size());
        if (esp_get_minimum_free_heap_size() < 2000) esp_restart();
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
