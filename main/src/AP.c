#include <string.h>

#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_wifi.h>

#include "include/AP.h"
#include "include/I2C.h"

void wifi_init_softap(void) {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize the Wi-Fi stack
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Configure the Access Point
    wifi_config_t wifi_config = {
        .ap = {
            .channel = 1,
            .password = WIFI_PASS,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };

    char buffer[strlen(WIFI_SSID) + 2 + 16 + 1 + 1]; // ": " + uint16_t, + "%" + "\0"
    uint16_t iCharge = read_2byte_data(STATE_OF_CHARGE_REG);
    snprintf(buffer, sizeof(buffer), "%s: %d%%", WIFI_SSID, iCharge);

    strncpy((char *)wifi_config.ap.ssid, buffer, sizeof(wifi_config.ap.ssid) - 1);
    wifi_config.ap.ssid[sizeof(wifi_config.ap.ssid) - 1] = '\0'; // Ensure null-termination
    wifi_config.ap.ssid_len = strlen((char *)wifi_config.ap.ssid); // Set SSID length

    if (strlen(WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP)); // Set ESP32 as AP
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI("AP", "WiFi AP started. SSID: %s, Password: %s", WIFI_SSID, WIFI_PASS);
}