#include "include/MESH.h"

#include "include/AP.h"
#include "include/BMS.h"
#include "include/global.h"
#include "include/WS.h"
#include "include/utils.h"

#include <string.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_websocket_client.h>

static esp_websocket_client_handle_t ws_client = NULL;
static char* mesh_ws_auth_token = "";

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
                vTaskSuspend(mesh_websocket_task_handle);

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

        if (data_string != NULL) {
            char uri[40+UTILS_AUTH_TOKEN_LENGTH];
            snprintf(uri, sizeof(uri), "ws://192.168.4.1:80/mesh_ws?auth_token=%s", mesh_ws_auth_token);
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
                char message[WS_MESSAGE_MAX_LEN];
                snprintf(message, sizeof(message), "{\"type\":\"data\",\"id\":\"%s\",\"content\":%s}", ESP_ID, data_string);
                esp_websocket_client_send_text(ws_client, message, strlen(message), portMAX_DELAY);
            }
        }

        // clean up
        free(data_string);
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
}

esp_err_t ap_n_client_comparison_handler(esp_http_client_event_t *evt) {
    static char *output_buffer;  // Buffer to store response
    static int output_len;       // Length of valid content

    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // Copy into output_buffer
                if (output_buffer == NULL) {
                    output_buffer = malloc(MESH_MAX_HTTP_RECV_BUFFER);
                    output_len = 0;
                }
                int copy_len = evt->data_len;
                if (output_len + copy_len < MESH_MAX_HTTP_RECV_BUFFER) {
                    memcpy(output_buffer + output_len, evt->data, copy_len);
                    output_len += copy_len;
                }
            }
            break;

        case HTTP_EVENT_ON_FINISH:
            if (output_buffer != NULL) {
                output_buffer[output_len] = '\0';
                ESP_LOGI(TAG, "Full response: %s", output_buffer);

                cJSON *message = cJSON_Parse(output_buffer);
                if (message) {
                    printf("woohoo\n");
                    cJSON *ext_num_object = cJSON_GetObjectItem(message, "num_connected_clients");
                    if (ext_num_object) {
                        int ext_num = ext_num_object->valueint;
                        printf("the number is %d\n", ext_num);
                        if (ext_num - 1 >= num_connected_clients) {
                            // -1 to account for the connection of this ESP32 to the other root AP
                            printf("I will shut down!\n");
                            esp_restart();
                        } else {
                            esp_http_client_config_t config = {
                                .url = "http://192.168.4.1/no_you_restart",
                            };
                            esp_http_client_handle_t client = esp_http_client_init(&config);
                            esp_http_client_set_method(client, HTTP_METHOD_POST);
                            esp_err_t err = esp_http_client_perform(client);
                            if (err == ESP_OK) {
                                ESP_LOGI("HTTP_CLIENT", "POST Status = %d",
                                        esp_http_client_get_status_code(client));
                            } else {
                                ESP_LOGE("HTTP_CLIENT", "POST failed: %s", esp_err_to_name(err));
                            }
                            esp_http_client_cleanup(client);
                        }
                    }
                }
                free(output_buffer);
                output_buffer = NULL;
                output_len = 0;
            }
            break;

        default:
            break;
    }
    return ESP_OK;
}

void merge_root_task(void *pvParameters) {
    while (true) {
        uint8_t my_mac[6];
        esp_wifi_get_mac(WIFI_IF_AP, my_mac);
        printf("My BSSID: %02x:%02x:%02x:%02x:%02x:%02x\n",
            my_mac[0], my_mac[1], my_mac[2],
            my_mac[3], my_mac[4], my_mac[5]);

        wifi_ap_record_t * ap_info = wifi_scan();
        if (ap_info) {
            printf("another root AP found!!\n SSID: %s, BSSID: %02x:%02x:%02x:%02x:%02x:%02x\n",
                ap_info->ssid,
                ap_info->bssid[0], ap_info->bssid[1], ap_info->bssid[2],
                ap_info->bssid[3], ap_info->bssid[4], ap_info->bssid[5]);

            int my_int = compare_mac(my_mac, ap_info->bssid);
            if (my_int < 0) {
                printf("I will change IP\n");
            } else if(my_int > 0) {
                printf("I will NOT change IP\n");
            } else {
                printf("major error\n");
            }

            wifi_config_t *wifi_sta_config = malloc(sizeof(wifi_config_t));
            memset(wifi_sta_config, 0, sizeof(wifi_config_t));

            strncpy((char *)wifi_sta_config->sta.ssid, (char *)ap_info->ssid, sizeof(wifi_sta_config->sta.ssid) - 1);
            wifi_sta_config->ap.authmode = WIFI_AUTH_OPEN;
            ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_sta_config));
            ESP_LOGI(TAG, "Connecting to AP... SSID: %s", wifi_sta_config->sta.ssid);

            uint8_t tries = 0;
            uint8_t max_tries = 10;
            bool connected = false;
            while (tries < max_tries) {
                esp_err_t err = esp_wifi_connect();
                if (err == ESP_OK) {
                    ESP_LOGI(TAG, "Success, waiting for connection...");
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    connected = true;
                    break;
                } else {
                    ESP_LOGW(TAG, "Failed to connect: %s. Retrying...", esp_err_to_name(err));
                    tries++;
                }
            }

            free(wifi_sta_config);

            if (connected) {
                // change to another IP so we can communicate with other root AP on it's default IP
                esp_netif_ip_info_t ip_info;
                if (my_int < 0) {
                    IP4_ADDR(&ip_info.ip, 192, 168, 10, 1);
                    IP4_ADDR(&ip_info.gw, 192, 168, 10, 1);
                    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);
                    esp_netif_dhcps_stop(ap_netif);
                    esp_netif_set_ip_info(ap_netif, &ip_info);
                    esp_netif_dhcps_start(ap_netif);
                    printf("changed IP to 192.168.10.1\n");
                    // delay to allow it to update
                    vTaskDelay(pdMS_TO_TICKS(5000));
                }


                // fetch the number of clients connected to the other AP
                esp_http_client_config_t config = {
                    .url = "http://192.168.4.1/api_num_clients",
                    .event_handler = ap_n_client_comparison_handler,
                };
                esp_http_client_handle_t client = esp_http_client_init(&config);
                esp_err_t err = esp_http_client_perform(client);
                if (err == ESP_OK) {
                    ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %lld",
                            esp_http_client_get_status_code(client),
                            esp_http_client_get_content_length(client));
                } else {
                    ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
                }
                esp_http_client_cleanup(client);


                if (my_int < 0) {
                    // change back to default
                    IP4_ADDR(&ip_info.ip, 192, 168, 4, 1);  // Change to a temporary IP
                    IP4_ADDR(&ip_info.gw, 192, 168, 4, 1);  // Gateway IP
                    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);  // Subnet mask
                    esp_netif_dhcps_stop(ap_netif);
                    esp_netif_set_ip_info(ap_netif, &ip_info);
                    esp_netif_dhcps_start(ap_netif);
                    printf("changed IP back to 192.168.4.1\n");
                }

                esp_wifi_disconnect();
            }
        }


        vTaskDelay(pdMS_TO_TICKS(60000));
    }
}
