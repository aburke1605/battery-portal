#include "include/AP.h"
#include "include/utils.h"

#include <stdint.h>
#include <stdbool.h>
#include "esp_netif_types.h"

esp_netif_t *ap_netif;
bool is_root = false;
int num_connected_clients = 0;
uint8_t ESP_ID = 0;

void app_main(void) {
    initialise_nvs();

    wifi_init();
}
