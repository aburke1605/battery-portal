#include <string.h>
#include <ctype.h>

#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_wifi.h>

#include "include/AP.h"

void wifi_scan(void) {
    // Configure Wi-Fi scan settings
    wifi_scan_config_t scan_config = {
        .ssid = NULL,        // Scan all SSIDs
        .bssid = NULL,       // Scan all BSSIDs
        .channel = 0,        // Scan all channels
        .show_hidden = false // Don't include hidden networks
    };

    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true)); // Blocking scan

    // Get the number of APs found
    uint16_t ap_num = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_num));
    ESP_LOGI("AP", "Number of access points found: %d", ap_num);

    // Allocate memory for AP info and retrieve the list
    wifi_ap_record_t *ap_info = malloc(sizeof(wifi_ap_record_t) * ap_num);
    if (ap_info == NULL) {
        ESP_LOGE("AP", "Failed to allocate memory for AP list");
        return;
    }

    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_num, ap_info));

    const char *target_substring = "AceOn battery";
    size_t target_length = strlen(target_substring);

    // Print out AP details
    for (int i = 0; i < ap_num; i++) {
        const char* ssid = (const char *)ap_info[i].ssid;
        if (strstr(ssid, target_substring) != NULL) {
            // Calculate the start of the trailing part
            const char *trailing_part = ssid + target_length;

            // Skip leading spaces
            while (*trailing_part == ' ') trailing_part++;

            // Check if the trailing part starts with digits
            if (isdigit((unsigned char)*trailing_part)) {
                // Extract the digits
                char digits[4] = {0}; // Assuming numbers won't exceed 3 digits
                int j = 0;
                while (isdigit((unsigned char)*trailing_part) && j < sizeof(digits) - 1) digits[j++] = *trailing_part++;
                digits[j] = '\0'; // Null-terminate the string

                ESP_LOGI("AP", "SSID: %s, Trailing Number: %s", ssid, digits);
            } else {
                ESP_LOGW("AP", "SSID: %s contains '%s' but no trailing number", ssid, target_substring);
            }
        }
    }
    free(ap_info);
}

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

    wifi_scan();
    printf("here\n\n");
    ESP_ERROR_CHECK(esp_wifi_stop());

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
        .addr = esp_ip4addr_aton("192.168.5.1"), // AceOn battery 1(=4-3)
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
    ESP_LOGI("AP", "AP initialized with IP: 192.168.5.1");


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