#include "esp_netif.h"

void wifi_init(void) {
    esp_netif_init();
}

void app_main(void) {
    wifi_init();
}
