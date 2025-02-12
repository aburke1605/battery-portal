#include <esp_spiffs.h>

#include "include/AP.h"
#include "include/DNS.h"
#include "include/I2C.h"
#include "include/ping.h"
#include "include/WS.h"
#include "include/utils.h"

// global variables
httpd_handle_t server = NULL;
int client_sockets[CONFIG_MAX_CLIENTS];
char received_data[1024];
SemaphoreHandle_t data_mutex;
bool connected_to_WiFi = false;
int other_AP_SSIDs[256];
char successful_ips[256][16];
char old_successful_ips[256][16];
uint8_t successful_ip_count = 0;
uint8_t old_successful_ip_count = 0;
char ESP_IP[16] = "xxx.xxx.xxx.xxx\0";

void app_main(void) {
    // initialise SPIFFS
    esp_err_t result;

    esp_vfs_spiffs_conf_t config_static = {
        .base_path = "/static",
        .partition_label = "staticstorage",
        .max_files = 5,
        .format_if_mount_failed = true
    };
    result = esp_vfs_spiffs_register(&config_static);

    esp_vfs_spiffs_conf_t config_templates = {
        .base_path = "/templates",
        .partition_label = "templatesstorage",
        .max_files = 5,
        .format_if_mount_failed = true
    };
    result |= esp_vfs_spiffs_register(&config_templates);

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

    xTaskCreate(&web_task, "web_task", 4096, &server, 5, NULL);

    xTaskCreate(&get_devices_task, "get_devices_task", 4096, NULL, 5, NULL); // TODO use ARP table

    esp_log_level_set("wifi", ESP_LOG_ERROR);
    xTaskCreate(&check_wifi_task, "check_wifi_task", 4096, NULL, 5, NULL);
}
