#include "include/AP.h"

#include <stdint.h>
#include "esp_netif_types.h"

esp_netif_t *ap_netif;
uint8_t ESP_ID = 0;

void app_main(void) {
    wifi_init();
}
