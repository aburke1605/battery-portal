#include "include/AP.h"
#include "include/I2C.h"
#include "include/config.h"
#include "include/utils.h"

#include <stdint.h>
#include <stdbool.h>
#include "esp_netif_types.h"
#include "esp_log.h"

esp_netif_t *ap_netif;
bool is_root = false;
int num_connected_clients = 0;
uint8_t ESP_ID = 0;

void app_main(void) {
    initialise_nvs();

    initialise_spiffs();

    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI("main", "I2C initialized successfully");
    if (SCAN_I2C) device_scan();

    wifi_init();
}
