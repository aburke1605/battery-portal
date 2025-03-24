#include "include/AP.h"

#include "include/global.h"
#include "include/config.h"
#include "include/I2C.h"

#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_wifi.h>

void wifi_init(void) {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    // Initialize the Wi-Fi stack
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    esp_netif_create_default_wifi_sta();
    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // scan for avail wifi networks first
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_ERROR_CHECK(esp_wifi_stop());

    // Set Wi-Fi mode to both AP and STA
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    // Configure the Access Point
    wifi_config_t wifi_ap_config = {
        .ap = {
            .channel = 1,
            .max_connection = AP_MAX_STA_CONN,
            .authmode = WIFI_AUTH_OPEN
        },
    };

    // set the SSID as well
    uint8_t name[11];
    read_bytes(I2C_DATA_SUBCLASS_ID, I2C_NAME_OFFSET
        + 1 // what's this about????
    , name, sizeof(name));
    strncpy(ESP_ID, (char *)name, 10);

    uint8_t charge[2] = {};
    read_bytes(0, I2C_STATE_OF_CHARGE_REG, charge, sizeof(charge));

    char buffer[strlen(ESP_ID) + 2 + 5 + 1 + 1]; // "BMS_01" + ": " + uint16_t, + "%" + "\0"
    snprintf(buffer, sizeof(buffer), "%s: %d%%", ESP_ID, charge[1] << 8 | charge[0]);

    strncpy((char *)wifi_ap_config.ap.ssid, buffer, sizeof(wifi_ap_config.ap.ssid) - 1);
    wifi_ap_config.ap.ssid[sizeof(wifi_ap_config.ap.ssid) - 1] = '\0'; // Ensure null-termination
    wifi_ap_config.ap.ssid_len = strlen((char *)wifi_ap_config.ap.ssid); // Set SSID length

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_ap_config));


    // manually set the netmask and gateway as something different from first ESP
    ESP_ERROR_CHECK(esp_netif_dhcps_stop(ap_netif));
    snprintf(ESP_subnet_IP, sizeof(ESP_subnet_IP), "192.168.4.1");
    esp_ip4_addr_t ip_info = {
        .addr = esp_ip4addr_aton(ESP_subnet_IP),
    };
    esp_ip4_addr_t netmask = {
        .addr = esp_ip4addr_aton("255.255.255.0"),
    };

    esp_ip4_addr_t gateway = {
        .addr = esp_ip4addr_aton(ESP_subnet_IP),
    };
    esp_netif_ip_info_t ip_info_struct;
    ip_info_struct.ip = ip_info;
    ip_info_struct.netmask = netmask;
    ip_info_struct.gw = gateway;
    ESP_ERROR_CHECK(esp_netif_set_ip_info(ap_netif, &ip_info_struct));
    ESP_ERROR_CHECK(esp_netif_dhcps_start(ap_netif));
    ESP_LOGI("AP", "AP initialized with IP: %s", ESP_subnet_IP);

    ESP_LOGI("AP", "Starting WiFi AP... SSID: %s", wifi_ap_config.ap.ssid);
    ESP_ERROR_CHECK(esp_wifi_start());
}
