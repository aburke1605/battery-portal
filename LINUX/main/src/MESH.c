#include "include/MESH.h"

#include "include/global.h"
#include "include/utils.h"
#include "include/AP.h"
#include "include/BMS.h"
#include "include/WS.h"

#include <stdbool.h>
#include "esp_wifi_types_generic.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_websocket_client.h"

static const char* TAG = "MESH";

static esp_websocket_client_handle_t ws_client = NULL;
static char* mesh_ws_auth_token = "";

void connect_to_root_task(void *pvParameters) {
    while (true) {
        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) { // if no wifi connection
            bool reconnected = false;
            
            // initial scan for other ROOT APs
            ESP_ERROR_CHECK(esp_wifi_stop());
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
            ESP_ERROR_CHECK(esp_wifi_start());
            vTaskDelay(pdMS_TO_TICKS(100));
            wifi_ap_record_t *ap_info = wifi_scan();
            ESP_ERROR_CHECK(esp_wifi_stop());
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
            ESP_ERROR_CHECK(esp_wifi_start());

            if (ap_info) {
                // try to connect to ROOT AP
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
                        connected_to_root = true;
                        reconnected = true;
                        vTaskDelay(pdMS_TO_TICKS(1000));
                        break;
                    } else {
                        ESP_LOGW(TAG, "Failed to connect: %s. Retrying...", esp_err_to_name(err));
                        connected_to_root = false;
                        tries++;
                    }
                }

                free(wifi_sta_config);
            }

            if (!reconnected) {
                connected_to_root = false;
                vTaskSuspend(mesh_websocket_task_handle);

                // scan to double check there really is no existing ROOT AP
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
                    ESP_LOGI(TAG, "delaying for %d ms...", time_delay);
                    vTaskDelay(pdMS_TO_TICKS(time_delay));
                    ESP_LOGI(TAG, "trying for last time... ");
                    // try to connect to new ROOT AP which has possibly restarted earlier than this
                    ap_info = wifi_scan();
                    if (!ap_info) {
                        ESP_LOGI(TAG, "fail, restarting");
                        // hopefully become now the only ROOT AP
                        esp_restart();
                    }
                }

                ESP_ERROR_CHECK(esp_wifi_stop());
                ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
                ESP_ERROR_CHECK(esp_wifi_start());
                vTaskResume(mesh_websocket_task_handle);
            } else {
                // make sure the mesh ws client is "authenticated"
                vTaskDelay(pdMS_TO_TICKS(5000));
                mesh_ws_auth_token = send_fake_login_post_request();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void mesh_websocket_task(void *pvParameters) {
    while (true) {
        char *data_string = get_data();

        if (connected_to_root && data_string != NULL && strcmp(mesh_ws_auth_token, "") != 0) {
            char uri[40+UTILS_AUTH_TOKEN_LENGTH+11];
            snprintf(uri, sizeof(uri), "ws://192.168.4.1:80/mesh_ws?auth_token=%s&esp_id=%u", mesh_ws_auth_token, ESP_ID);
            const esp_websocket_client_config_t websocket_cfg = {
                .uri = uri,
                .reconnect_timeout_ms = 10000,
                .network_timeout_ms = 10000,
            };

            if (ws_client == NULL) {
                ws_client = esp_websocket_client_init(&websocket_cfg);
                if (ws_client == NULL) {
                    ESP_LOGE(TAG, "Failed to initialize WebSocket client");
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    continue;
                }
                esp_websocket_register_events(ws_client, WEBSOCKET_EVENT_ANY, websocket_event_handler, NULL);
                if (esp_websocket_client_start(ws_client) != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to start WebSocket client");
                    esp_websocket_client_destroy(ws_client);
                    ws_client = NULL;
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    continue;
                }
            }

            vTaskDelay(pdMS_TO_TICKS(5000));

            if (!esp_websocket_client_is_connected(ws_client)) {
                esp_websocket_client_stop(ws_client);
                esp_websocket_client_destroy(ws_client);
                ws_client = NULL;
            } else {
                esp_websocket_client_send_text(ws_client, data_string, strlen(data_string), portMAX_DELAY);
            }
        }

        // clean up
        free(data_string);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
