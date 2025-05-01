#include "include/config.h"
#include "include/global.h"
#include "include/AP.h"
#include "include/DNS.h"
#include "include/I2C.h"
#include "include/BMS.h"
#include "src/test.c"
#include "include/WS.h"
#include "include/utils.h"

#include <esp_log.h>
#include <esp_spiffs.h>
#include <driver/gpio.h>

// global variables
char ESP_ID[UTILS_ID_LENGTH + 1] = "unknown";
httpd_handle_t server = NULL;
bool connected_to_WiFi = false;
char ESP_subnet_IP[15];
client_socket client_sockets[WS_CONFIG_MAX_CLIENTS];
char current_auth_token[UTILS_AUTH_TOKEN_LENGTH] = "";
QueueHandle_t ws_queue;

TaskHandle_t websocket_task_handle = NULL;

void app_main(void) {
    /*
    uint8_t spreading_factor = 7;
    uint16_t bandwidth = 250; // kHz
    uint8_t n_preamble_symbols = 8; // default
    uint16_t payload_length = 175; // e.g.
    bool header = true;
    bool low_data_rate_optimisation = false;

    int transmission_delay;
    printf("calculate_transmission_delay(%d, %d, %d, %d, %d, %s, %s):\n", spreading_factor, bandwidth, n_preamble_symbols, payload_length, 1, header?"true":"false", low_data_rate_optimisation?"true":"false");
    transmission_delay = calculate_transmission_delay(spreading_factor, bandwidth, n_preamble_symbols, payload_length, 1, header, low_data_rate_optimisation);
    printf("transmission delay: %d\n\n\n", transmission_delay);
    vTaskDelay(pdMS_TO_TICKS(transmission_delay));

    printf("calculate_transmission_delay(%d, %d, %d, %d, %d, %s, %s):\n", spreading_factor, bandwidth, n_preamble_symbols, payload_length, 2, header?"true":"false", low_data_rate_optimisation?"true":"false");
    transmission_delay = calculate_transmission_delay(spreading_factor, bandwidth, n_preamble_symbols, payload_length, 2, header, low_data_rate_optimisation);
    printf("transmission delay: %d\n\n\n", transmission_delay);
    vTaskDelay(pdMS_TO_TICKS(transmission_delay));

    printf("calculate_transmission_delay(%d, %d, %d, %d, %d, %s, %s):\n", spreading_factor, bandwidth, n_preamble_symbols, payload_length, 3, header?"true":"false", low_data_rate_optimisation?"true":"false");
    transmission_delay = calculate_transmission_delay(spreading_factor, bandwidth, n_preamble_symbols, payload_length, 3, header, low_data_rate_optimisation);
    printf("transmission delay: %d\n\n\n", transmission_delay);
    vTaskDelay(pdMS_TO_TICKS(transmission_delay));

    printf("calculate_transmission_delay(%d, %d, %d, %d, %d, %s, %s):\n", spreading_factor, bandwidth, n_preamble_symbols, payload_length, 4, header?"true":"false", low_data_rate_optimisation?"true":"false");
    transmission_delay = calculate_transmission_delay(spreading_factor, bandwidth, n_preamble_symbols, payload_length, 4, header, low_data_rate_optimisation);
    printf("transmission delay: %d\n\n\n", transmission_delay);
    vTaskDelay(pdMS_TO_TICKS(transmission_delay));
    */

    // Reset pin
    printf("1\n");
    gpio_set_direction(PIN_NUM_RST, GPIO_MODE_OUTPUT);
    printf("2\n");
    gpio_set_level(PIN_NUM_RST, 1);
    printf("3\n");

    // SPI bus config
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };
    ESP_ERROR_CHECK(spi_bus_initialize(HSPI_HOST, &buscfg, SPI_DMA_DISABLED));
    printf("4\n");

    // SPI device config
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1 * 1000 * 1000, // 1 MHz
        .mode = 0,
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 1,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(HSPI_HOST, &devcfg, &spi));
    printf("5\n");

    rfm95_reset();
    printf("6\n");
        // --- SPI Communication Test ---
        uint8_t version = rfm95_read(0x42);  // REG_VERSION
        if (version == 0x12) {
            ESP_LOGI(TAG, "RFM95W detected (version 0x%02X)", version);
        } else {
            ESP_LOGE(TAG, "Failed to detect RFM95W! REG_VERSION = 0x%02X", version);
            return; // Stop if no communication
        }
    rfm95_configure();
    printf("7\n");

    const char* msg = "Hello, LoRa!";
    rfm95_send_packet((const uint8_t*)msg, strlen(msg));

    printf("finished\n");

    return;

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

    // do a BMS reset on boot
    reset();

    vTaskDelay(pdMS_TO_TICKS(1000));

    // Initialize the GPIO pin as an output for LED toggling
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << I2C_LED_GPIO_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    uint8_t eleven_bytes[11];
    read_bytes(I2C_DATA_SUBCLASS_ID, I2C_NAME_OFFSET, eleven_bytes, sizeof(eleven_bytes));
    if (strcmp((char *)eleven_bytes, "") != 0) strncpy(ESP_ID, (char *)eleven_bytes, 10);

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

    TaskParams dns_server_params = {.stack_size = 2500, .task_name = "dns_server_task"};
    xTaskCreate(&dns_server_task, dns_server_params.task_name, dns_server_params.stack_size, &dns_server_params, 2, NULL);

    ws_queue = xQueueCreate(WS_QUEUE_SIZE, WS_MESSAGE_MAX_LEN);
    // ws_queue = xQueueCreate(WS_QUEUE_SIZE, sizeof(ws_payload));
    TaskParams message_queue_params = {.stack_size = 3200, .task_name = "message_queue_task"};
    xTaskCreate(&message_queue_task, message_queue_params.task_name, message_queue_params.stack_size, &message_queue_params, 5, NULL);

    esp_log_level_set("wifi", ESP_LOG_ERROR);
    esp_log_level_set("websocket_client", ESP_LOG_WARN);
    esp_log_level_set("transport_ws", ESP_LOG_WARN);
    esp_log_level_set("transport_base", ESP_LOG_WARN);
    TaskParams websocket_params = {.stack_size = 4600, .task_name = "websocket_task"};
    xTaskCreate(&websocket_task, websocket_params.task_name, websocket_params.stack_size, &websocket_params, 1, &websocket_task_handle);

    while (true) {
        if (VERBOSE) ESP_LOGI("main", "%ld bytes available in heap", esp_get_free_heap_size());
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
