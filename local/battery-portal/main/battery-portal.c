#include "include/AP.h"
#include "include/BMS.h"
#include "include/GPS.h"
#include "include/I2C.h"
#include "include/config.h"
#include "include/utils.h"

#include <stdint.h>
#include <stdbool.h>
#include "esp_netif_types.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

esp_netif_t *ap_netif;
bool is_root = false;
int num_connected_clients = 0;
uint8_t ESP_ID = 0;

TaskHandle_t websocket_task_handle = NULL;

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
        /*
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

        // grab BMS DeviceName from the BMS DataFlash
        uint8_t address[2] = {0};
        convert_uint_to_n_bytes(I2C_DEVICE_NAME_ADDR, address, sizeof(address), true);
        uint8_t data_flash[UTILS_ID_LENGTH + 1] = {0}; // S21 data type
        read_data_flash(address, sizeof(address), data_flash, sizeof(data_flash));
        // store the ID in ESP32 memory
        if (strcmp((char *)data_flash, "") != 0)
            change_esp_id((char*)&data_flash[1]);
        */
    }

    wifi_init();
}
