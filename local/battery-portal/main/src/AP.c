#include "include/AP.h"

#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi_default.h"

void wifi_init(void) {
    // initialize the Wi-Fi stack
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    esp_netif_create_default_wifi_sta();
    /*

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // use only STA at first to scan for APs
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    vTaskDelay(pdMS_TO_TICKS(100));

    // scan for other WiFi APs
    wifi_ap_record_t * AP_exists = wifi_scan();

    // stop WiFi before changing mode
    ESP_ERROR_CHECK(esp_wifi_stop());

    // create the AP network interface
    ap_netif = esp_netif_create_default_wifi_ap();

    // set Wi-Fi mode to both AP and STA
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    // configure the Access Point
    wifi_config_t wifi_ap_config = {
        .ap = {
            .channel = 1,
            .max_connection = AP_MAX_STA_CONN,
            .authmode = WIFI_AUTH_OPEN
        },
    };

    // set the SSID as well
    char buffer[5 + 8 + 2 + 5 + 1 + 1]; // "ROOT " + "BMS_255" + ": " + uint16_t, + "%" + "\0"
    if (LORA_IS_RECEIVER) {
        snprintf(buffer, sizeof(buffer), "LoRa RECEIVER");
    } else {
        uint8_t data_SBS[2] = {0};
        read_SBS_data(I2C_RELATIVE_STATE_OF_CHARGE_ADDR, data_SBS, sizeof(data_SBS));
        snprintf(buffer, sizeof(buffer), "%sbms_%02u: %d%%", !AP_exists?"ROOT ":"", ESP_ID, data_SBS[1] << 8 | data_SBS[0]);
    }

    strncpy((char *)wifi_ap_config.ap.ssid, buffer, sizeof(wifi_ap_config.ap.ssid) - 1);
    wifi_ap_config.ap.ssid[sizeof(wifi_ap_config.ap.ssid) - 1] = '\0'; // ensure null-termination
    wifi_ap_config.ap.ssid_len = strlen((char *)wifi_ap_config.ap.ssid); // set SSID length

    // specific configurations or ROOT or non-ROOT APs
    if (!AP_exists) {
        // keep count of number of connected clients
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &ap_n_clients_handler, NULL));
        is_root = true;
    } else {
        // must change IP address from default so
        // can send messages to ROOT at 192.168.4.1
        esp_netif_ip_info_t ip_info;
        IP4_ADDR(&ip_info.ip, 192,168,5,1);
        IP4_ADDR(&ip_info.gw, 192,168,5,1);
        IP4_ADDR(&ip_info.netmask, 255,255,255,0);
        esp_netif_dhcps_stop(ap_netif);
        esp_netif_set_ip_info(ap_netif, &ip_info);
        esp_netif_dhcps_start(ap_netif);
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_ap_config));

    // restart WiFi
    ESP_LOGI(TAG, "Starting WiFi AP... SSID: %s", wifi_ap_config.ap.ssid);
    ESP_ERROR_CHECK(esp_wifi_start());
    vTaskDelay(pdMS_TO_TICKS(100));
    */
}
