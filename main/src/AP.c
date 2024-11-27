#include <string.h>

#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_wifi.h>

#include "include/AP.h"

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

    // Set Wi-Fi mode to both AP and STA
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    // Configure the Access Point
    wifi_config_t wifi_ap_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .channel = 1,
            .password = WIFI_PASS,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    if (strlen(WIFI_PASS) == 0) {
        wifi_ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_ap_config));


    // manually set the netmask and gateway as something different from first ESP
    // TODO: can we somehow 'discover' the available APs and their IPs,
    //       and automatically choose a unique one here?
    ESP_ERROR_CHECK(esp_netif_dhcps_stop(ap_netif));
    esp_ip4_addr_t ip_info = {
        .addr = esp_ip4addr_aton("192.168.5.1"),
    };
    esp_ip4_addr_t netmask = {
        .addr = esp_ip4addr_aton("255.255.255.0"),
    };

    esp_ip4_addr_t gateway = {
        .addr = esp_ip4addr_aton("192.168.5.1"),
    };
    esp_netif_ip_info_t ip_info_struct;
    ip_info_struct.ip = ip_info;
    ip_info_struct.netmask = netmask;
    ip_info_struct.gw = gateway;
    ESP_ERROR_CHECK(esp_netif_set_ip_info(ap_netif, &ip_info_struct));
    ESP_ERROR_CHECK(esp_netif_dhcps_start(ap_netif));
    ESP_LOGI("wifi_init_ap", "AP initialized with IP: 192.168.5.1");


    wifi_config_t wifi_sta_config = {
        .sta = {
            .ssid = "AceOn battery",
            .password = "password",
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_sta_config));

    ESP_LOGI("AP", "Starting WiFi AP... SSID: %s, Password: %s", WIFI_SSID, WIFI_PASS);
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI("AP", "Connecting to AP... SSID: %s", wifi_sta_config.sta.ssid);
    ESP_ERROR_CHECK(esp_wifi_connect());
}