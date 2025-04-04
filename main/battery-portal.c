#include <esp_spiffs.h>

#include "include/config.h"
#include "include/AP.h"
#include "include/DNS.h"
#include "include/I2C.h"
#include "include/ARP.h"
#include "include/WS.h"
#include "include/utils.h"

// global variables
char ESP_ID[UTILS_KEY_LENGTH + 1];
httpd_handle_t server = NULL;
int client_sockets[WS_CONFIG_MAX_CLIENTS];
char received_data[1024];
SemaphoreHandle_t data_mutex;
bool connected_to_WiFi = false;
bool reconnect = false;
int other_AP_SSIDs[256];
char successful_ips[256][16];
char old_successful_ips[256][16];
uint8_t successful_ip_count = 0;
uint8_t old_successful_ip_count = 0;
char ESP_IP[16] = "xxx.xxx.xxx.xxx\0";
char ESP_subnet_IP[15];
esp_websocket_client_handle_t ws_client = NULL;
QueueHandle_t ws_queue;
struct rendered_page rendered_html_pages[WS_MAX_N_HTML_PAGES];
uint8_t n_rendered_html_pages = 0;
bool admin_verified = false;
i2c_master_bus_handle_t i2c_bus = NULL;
i2c_master_dev_handle_t i2c_device = NULL;

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

    // Initialize the GPIO pin as an output for LED toggling
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << I2C_LED_GPIO_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    read_name(I2C_DATA_SUBCLASS_ID, I2C_NAME_OFFSET, ESP_ID);

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

    xTaskCreate(&dns_server_task, dns_server_params.task_name, dns_server_params.stack_size, &dns_server_params, 5, NULL);
    TaskParams dns_server_params = {.stack_size = 2500, .task_name = "dns_server_task"};

    esp_log_level_set("wifi", ESP_LOG_ERROR);
    TaskParams check_wifi_params = {.stack_size = 2048, .task_name = "check_wifi_task"};
    xTaskCreate(&check_wifi_task, check_wifi_params.task_name, check_wifi_params.stack_size, &check_wifi_params, 5, NULL);

    ws_queue = xQueueCreate(WS_QUEUE_SIZE, WS_MESSAGE_MAX_LEN);
    TaskParams message_queue_params = {.stack_size = 4096, .task_name = "message_queue_task"};
    xTaskCreate(message_queue_task, message_queue_params.task_name, message_queue_params.stack_size, &message_queue_params, 5, NULL);

    esp_log_level_set("websocket_client", ESP_LOG_WARN);
    esp_log_level_set("transport_ws", ESP_LOG_WARN);
    esp_log_level_set("transport_base", ESP_LOG_WARN);
    TaskParams websocket_params = {.stack_size = 8192, .task_name = "websocket_task"};
    xTaskCreate(&websocket_task, websocket_params.task_name, websocket_params.stack_size, &websocket_params, 5, NULL);

    while (true) {
        if (VERBOSE) ESP_LOGI("main", "%ld bytes available in heap", esp_get_free_heap_size());
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
