#include "include/MESH.h"

#include "include/AP.h"

#include <string.h>
#include <esp_log.h>
#include <esp_wifi.h>

static const char* TAG = "MESH";

void connect_to_root_task(void *pvParameters) {
    while (true) {
        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) { // if no wifi connection
            bool reconnected = false;
            
            // initial scan for other root APs
            ESP_ERROR_CHECK(esp_wifi_stop());
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
            ESP_ERROR_CHECK(esp_wifi_start());
            vTaskDelay(pdMS_TO_TICKS(100));
            wifi_ap_record_t *ap_info = wifi_scan();
            ESP_ERROR_CHECK(esp_wifi_stop());
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
            ESP_ERROR_CHECK(esp_wifi_start());

            if (ap_info) {
                // try to connect to root AP
                wifi_config_t *wifi_sta_config = malloc(sizeof(wifi_config_t));
                memset(wifi_sta_config, 0, sizeof(wifi_config_t));

                strncpy((char *)wifi_sta_config->sta.ssid, (char *)ap_info->ssid, sizeof(wifi_sta_config->sta.ssid) - 1);
                wifi_sta_config->ap.authmode = WIFI_AUTH_OPEN;
                ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_sta_config));
                ESP_LOGI(TAG, "Connecting to AP... SSID: %s", wifi_sta_config->sta.ssid);

                uint8_t tries = 0;
                uint8_t max_tries = 10;
                while (tries < max_tries) {
                    esp_err_t err = esp_wifi_connect();
                    if (err == ESP_OK) {
                        ESP_LOGI(TAG, "Success, waiting for connection...");
                        reconnected = true;
                        vTaskDelay(pdMS_TO_TICKS(1000));
                        break;
                    } else {
                        ESP_LOGW(TAG, "Failed to connect: %s. Retrying...", esp_err_to_name(err));
                        tries++;
                    }
                }

                free(wifi_sta_config);
            }

            if (!reconnected) {
                vTaskSuspend(websocket_message_task_handle);

                // scan to double check there really is no existing root AP
                ESP_ERROR_CHECK(esp_wifi_stop());
                ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
                ESP_ERROR_CHECK(esp_wifi_start());
                vTaskDelay(pdMS_TO_TICKS(100));
                wifi_ap_record_t *ap_info = wifi_scan();

                // if not, nominate a new one using MAC addresses
                if (!ap_info) {
                    uint8_t my_mac[6];
                    esp_wifi_get_mac(WIFI_IF_AP, my_mac);
                    uint32_t unique_number = ((uint32_t)my_mac[2] << 24) |
                                             ((uint32_t)my_mac[3] << 16) |
                                             ((uint32_t)my_mac[4] << 8)  |
                                             ((uint32_t)my_mac[5]);
                    int time_delay = unique_number % 100000 + 5000;
                    printf("delaying for %d ms...\n", time_delay);
                    vTaskDelay(pdMS_TO_TICKS(time_delay));
                    printf("trying for last time... ");
                    // try to connect to new root AP which has possibly restarted earlier than this
                    ap_info = wifi_scan();
                    if (!ap_info) {
                        printf("fail, restarting\n");
                        // hopefully become now the only root AP
                        esp_restart();
                    }
                }

                ESP_ERROR_CHECK(esp_wifi_stop());
                ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
                ESP_ERROR_CHECK(esp_wifi_start());
                vTaskResume(websocket_message_task_handle);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}