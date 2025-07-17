#include "include/WS.h"

#include "include/I2C.h"
#include "include/BMS.h"
#include "include/global.h"
#include "include/utils.h"

#include "include/cert.h"
#include "include/local_cert.h"

#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_websocket_client.h>
#include <esp_eap_client.h>
#include <driver/gpio.h>

static bool reconnect = false;
static char ESP_IP[16] = "xxx.xxx.xxx.xxx\0";
static esp_websocket_client_handle_t ws_client = NULL;

static const char* TAG = "WS";

void add_client(int fd, const char* tkn, bool browser) {
    for (int i = 0; i < WS_CONFIG_MAX_CLIENTS; i++) {
        if (client_sockets[i].descriptor == fd) {
            return;
        } else if (client_sockets[i].descriptor < 0) {
            client_sockets[i].descriptor = fd;
            strncpy(client_sockets[i].auth_token, tkn, UTILS_AUTH_TOKEN_LENGTH);
            client_sockets[i].auth_token[UTILS_AUTH_TOKEN_LENGTH - 1] = '\0';
            client_sockets[i].is_browser_not_mesh = browser;
            ESP_LOGI(TAG, "Client %d added", fd);
            return;
        }
    }
    ESP_LOGE(TAG, "No space for client %d", fd);
}

void remove_client(int fd) {
    for (int i = 0; i < WS_CONFIG_MAX_CLIENTS; i++) {
        if (client_sockets[i].descriptor == fd) {
            client_sockets[i].descriptor = -1;
            client_sockets[i].auth_token[0] = '\0';
            client_sockets[i].is_browser_not_mesh = true;
            ESP_LOGI(TAG, "Client %d removed", fd);
            return;
        }
    }
}

esp_err_t client_handler(httpd_req_t *req) {
    int fd = httpd_req_to_sockfd(req);

    // register new clients...
    bool is_browser_not_mesh = true; // arbitrary assumption
    if (req->method == HTTP_GET) {
        char *check_new_browser_session = strstr(req->uri, "/browser_ws?auth_token=");
        char *check_new_mesh_session = strstr(req->uri, "/mesh_ws?auth_token=");
        if (check_new_browser_session || check_new_mesh_session) {
            if (!check_new_browser_session && check_new_mesh_session) is_browser_not_mesh = false;
            char auth_token[UTILS_AUTH_TOKEN_LENGTH] = {0};
            if (is_browser_not_mesh) {
                sscanf(check_new_browser_session, "/browser_ws?auth_token=%50s", auth_token);
            } else {
                sscanf(check_new_mesh_session, "/mesh_ws?auth_token=%50s", auth_token);
            }
            auth_token[UTILS_AUTH_TOKEN_LENGTH - 1] = '\0';
            if (auth_token[0] != '\0' && strcmp(auth_token, current_auth_token) == 0) {
                add_client(fd, auth_token, is_browser_not_mesh);
                current_auth_token[0] = '\0';
                ESP_LOGI(TAG, "WebSocket handshake complete for client %d", fd);
                return ESP_OK; // WebSocket handshake happens here
            }
        }
        return ESP_FAIL;
    }

    // ...otherwise listen for messages
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    // first call with ws_pkt.len = 0 to determine required length
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to fetch WebSocket frame length: %s", esp_err_to_name(ret));
        return ret;
    }
    if (ws_pkt.len > 0) {
        // allocate buffer
        ws_pkt.payload = malloc(ws_pkt.len + 1);
        if (!ws_pkt.payload) {
            ESP_LOGE(TAG, "Failed to allocate memory for WebSocket payload");
            return ESP_ERR_NO_MEM;
        }
        // receive payload
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to receive WebSocket frame: %s", esp_err_to_name(ret));
            free(ws_pkt.payload);
            return ret;
        }
        ws_pkt.payload[ws_pkt.len] = '\0';
    }

    if (ws_pkt.type == HTTPD_WS_TYPE_TEXT) {
        if (VERBOSE) ESP_LOGI(TAG, "Received WebSocket message: %s", (char *)ws_pkt.payload);

        // read messaage data
        cJSON *message = cJSON_Parse((char *)ws_pkt.payload);
        if (!message) {
            ESP_LOGE(TAG, "Failed to parse JSON");
            free(ws_pkt.payload);
            return ESP_FAIL;
        }

        // re-determine if browser or mesh client
        for (int i = 0; i < WS_CONFIG_MAX_CLIENTS; i++) {
            if (client_sockets[i].descriptor == fd) {
                is_browser_not_mesh = client_sockets[i].is_browser_not_mesh;
                break;
            }
        }
        if (is_browser_not_mesh) {
            // perform the request made by the local websocket client
            cJSON *response = cJSON_CreateObject();
            perform_request(message, response);
        } else {
            // queue message from mesh client to forward via LoRa
            cJSON *id_obj = cJSON_GetObjectItem(message, "id");
            if (!id_obj) {
                ESP_LOGE(TAG, "incoming LoRa queue message not formatted properly:\n  %s", cJSON_PrintUnformatted(message));
                free(ws_pkt.payload);
                cJSON_Delete(message);
                return ESP_FAIL;
            }
            bool found = false;
            for (int i=0; i<MESH_SIZE; i++) {
                if (strcmp(all_messages[i].id, id_obj->valuestring) == 0) {
                    // update existing message
                    found = true;
                    strcpy(all_messages[i].message, (char *)ws_pkt.payload);
                    i = MESH_SIZE;
                } else if (strcmp(all_messages[i].id, "") == 0) {
                    // create new message
                    found = true;
                    strcpy(all_messages[i].id, id_obj->valuestring);
                    strcpy(all_messages[i].message, (char *)ws_pkt.payload);
                    i = MESH_SIZE;
                }
            }
            if (!found) ESP_LOGE(TAG, "LoRa queue full! Dropping message: %s", cJSON_PrintUnformatted(message));
        }

        cJSON_Delete(message);
    } else {
        ESP_LOGW(TAG, "Received unsupported WebSocket frame type: %d", ws_pkt.type);
    }

    free(ws_pkt.payload);

    return ESP_OK;
}

esp_err_t perform_request(cJSON *message, cJSON *response) {
    // construct response message
    cJSON_AddStringToObject(response, "type", "response");
    cJSON_AddStringToObject(response, "id", ESP_ID);
    cJSON *response_content = cJSON_CreateObject();

    cJSON *type = cJSON_GetObjectItem(message, "type");
    cJSON *content = cJSON_GetObjectItem(message, "content");
    if (!type || !content) {
        ESP_LOGE(TAG, "Error in request message");
        return ESP_FAIL;
    }

    if (strcmp(type->valuestring, "query") == 0) {
        if (strcmp(content->valuestring, "are you still there?") == 0) {
            cJSON_AddStringToObject(response_content, "response", "yes");
            xQueueReset(ws_queue); // prioritise this reply(?)
        }
    } else if (strcmp(type->valuestring, "request") == 0) {
        cJSON *summary = cJSON_GetObjectItem(content, "summary");
        if (summary && strcmp(summary->valuestring, "change-settings") == 0) {
            cJSON *data = cJSON_GetObjectItem(content, "data");
            if (!data) {
                ESP_LOGE(TAG, "Failed to parse JSON");
                cJSON_AddStringToObject(response_content, "status", "error");
                return ESP_FAIL;
            }

            gpio_set_level(I2C_LED_GPIO_PIN, 1);

            cJSON *esp_id = cJSON_GetObjectItem(data, "new_esp_id");
            if (esp_id && strcmp(esp_id->valuestring, "") != 0 && esp_id->valuestring != ESP_ID) {
                ESP_LOGI(TAG, "Changing device name...");

                uint8_t address[2] = {0};
                convert_uint_to_n_bytes(I2C_DEVICE_NAME_ADDR, address, sizeof(address), true);
                char* new_id = (char*)esp_id->valuestring;
                size_t new_id_length = strlen(new_id);
                if (new_id_length > 20) new_id_length = 20;
                uint8_t new_id_data[1+new_id_length];
                new_id_data[0] = new_id_length;
                for (size_t i=0; i<new_id_length; i++)
                    new_id_data[1 + i] = new_id[i];
                write_data_flash(address, sizeof(address), new_id_data, sizeof(new_id_data));
                strncpy(ESP_ID, (char *)&new_id_data[1], new_id_length);
                ESP_ID[new_id_length] = '\0';
            }

            int OTC_threshold = cJSON_GetObjectItem(data, "OTC_threshold")->valueint;

            uint8_t address[2] = {0};
            convert_uint_to_n_bytes(I2C_OTC_THRESHOLD_ADDR, address, sizeof(address), true);
            uint8_t data_flash[2] = {0};
            read_data_flash(address, sizeof(address), data_flash, sizeof(data_flash));
            if (OTC_threshold != (data_flash[1] << 8 | data_flash[0])) {
                ESP_LOGI(TAG, "Changing OTC threshold...");
                convert_uint_to_n_bytes(OTC_threshold, data_flash, sizeof(data_flash), false);
                write_data_flash(address, sizeof(address), data_flash, sizeof(data_flash));
            }

            gpio_set_level(I2C_LED_GPIO_PIN, 0);

            cJSON_AddStringToObject(response_content, "status", "success");
        } else if (summary && strcmp(summary->valuestring, "connect-wifi") == 0) {
            cJSON *data = cJSON_GetObjectItem(content, "data");
            if (!data) {
                ESP_LOGE(TAG, "Failed to parse JSON");
                cJSON_AddStringToObject(response_content, "status", "error");
                return ESP_FAIL;
            }

            const char* username = cJSON_GetObjectItem(data, "username")->valuestring;
            const char* password = cJSON_GetObjectItem(data, "password")->valuestring;
            cJSON *eduroam = cJSON_GetObjectItem(data, "eduroam");

            int tries = 0;
            int max_tries = 10;
            if (!connected_to_WiFi || reconnect) {
                if (reconnect) {
                    connected_to_WiFi = false;
                }

                wifi_config_t *wifi_sta_config = malloc(sizeof(wifi_config_t));
                memset(wifi_sta_config, 0, sizeof(wifi_config_t));

                if (cJSON_IsBool(eduroam) && cJSON_IsTrue(eduroam)) {
                    strncpy((char *)wifi_sta_config->sta.ssid, "eduroam", 8);

                    char full_username[48];
                    snprintf(full_username, sizeof(full_username), "%s@liverpool.ac.uk", username);
                    full_username[strlen(full_username)] = '\0';

                    esp_wifi_sta_enterprise_enable();
                    esp_eap_client_set_username((uint8_t *)full_username, strlen(full_username));
                    esp_eap_client_set_password((uint8_t *)password, strlen(password));
                    ESP_LOGI(TAG, "Connecting to AP... SSID: eduroam");
                } else {
                    strncpy((char *)wifi_sta_config->sta.ssid, username, sizeof(wifi_sta_config->sta.ssid) - 1);
                    if (strlen(password) == 0) {
                        wifi_sta_config->ap.authmode = WIFI_AUTH_OPEN;
                    } else {
                        strncpy((char *)wifi_sta_config->sta.password, password, sizeof(wifi_sta_config->sta.password) - 1);
                    }
                    ESP_LOGI(TAG, "Connecting to AP... SSID: %s", wifi_sta_config->sta.ssid);
                }

                ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_sta_config));
                // TODO: if reconnecting, it doesn't actually seem to drop the old connection in favour of the new one

                // give some time to connect
                vTaskDelay(pdMS_TO_TICKS(5000));
                while (true) {
                    if (tries > max_tries) break;
                    wifi_ap_record_t ap_info;
                    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {

                        esp_netif_t *sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
                        if (sta_netif != NULL) {
                            esp_netif_ip_info_t ip_info;
                            esp_netif_get_ip_info(sta_netif, &ip_info);

                            if (ip_info.ip.addr != IPADDR_ANY) {
                                connected_to_WiFi = true;
                                ESP_LOGI(TAG, "Connected to router. Signal strength: %d dBm", ap_info.rssi);
                                cJSON_AddStringToObject(response_content, "status", "success");

                                break;
                            }
                        }
                    } else {
                        if (VERBOSE) ESP_LOGI(TAG, "Not connected. Retrying... %d", tries);
                        esp_wifi_connect();
                    }
                    tries++;

                    vTaskDelay(pdMS_TO_TICKS(1000));
                }
            }
        } else if (summary && strcmp(summary->valuestring, "reset-bms") == 0) {
            reset();
            cJSON_AddStringToObject(response_content, "status", "success");
        } else if (summary && strcmp(summary->valuestring, "unseal-bms") == 0) {
            unseal();
            cJSON_AddStringToObject(response_content, "status", "success");
        }
    } else {
        return ESP_ERR_NOT_SUPPORTED;
    }
    cJSON_AddItemToObject(response, "content", response_content);
    return ESP_OK;
}

void send_message(const char *message) {
    if (xQueueSend(ws_queue, message, pdMS_TO_TICKS(100)) != pdPASS) {
        ESP_LOGE(TAG, "WebSocket queue full! Dropping message: %s", message);
    }
}
// void send_message(const ws_payload *message) {
//     if (xQueueSend(ws_queue, message, portMAX_DELAY) != pdPASS) {
//         ESP_LOGE(TAG, "WebSocket queue full! Dropping message.");
//     }
// }

void message_queue_task(void *pvParameters) {
    char message[WS_MESSAGE_MAX_LEN];
    // ws_payload message;

    while (true) {
        if (xQueueReceive(ws_queue, message, portMAX_DELAY) == pdPASS) {
        // if (xQueueReceive(ws_queue, &message, portMAX_DELAY) == pdPASS) {
            if (esp_websocket_client_is_connected(ws_client)) {
                if (VERBOSE) ESP_LOGI(TAG, "Sending: %s", message);
                esp_websocket_client_send_text(ws_client, message, strlen(message), portMAX_DELAY);
                // if (VERBOSE) ESP_LOGI(TAG, "Sending message to server...");
                // esp_websocket_client_send_bin(ws_client, (const char*)&message, sizeof(ws_payload), portMAX_DELAY);
            } else {
                ESP_LOGW(TAG, "WebSocket not connected, dropping message: %s", message);
                // ESP_LOGW(TAG, "WebSocket not connected, dropping message.");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void websocket_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    esp_websocket_event_data_t *ws_event_data = (esp_websocket_event_data_t *)event_data;

    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            if (VERBOSE) ESP_LOGI(TAG, "WebSocket connected");
            char websocket_connect_message[WS_MESSAGE_MAX_LEN];
            snprintf(websocket_connect_message, sizeof(websocket_connect_message), "{\"type\":\"register\",\"id\":\"%s\"}", ESP_ID);
            if (esp_websocket_client_is_connected(ws_client)) send_message(websocket_connect_message);
            break;

        case WEBSOCKET_EVENT_DISCONNECTED:
            if (VERBOSE) ESP_LOGI(TAG, "WebSocket disconnected");
            esp_websocket_client_stop(ws_client);
            break;

        case WEBSOCKET_EVENT_DATA:
            if (ws_event_data->data_len == 0) break;

            cJSON *message = cJSON_Parse((char *)ws_event_data->data_ptr);
            if (!message) {
                ESP_LOGE(TAG, "invalid json");
                cJSON_Delete(message);
                break;
            }

            // this case should never be triggered by the mesh ws
            // event handling, so just continue with the code below
            // for browser ws events and use this handler universally
            // for both mesh and browser ws clients

            cJSON *response = cJSON_CreateObject();
            esp_err_t err = perform_request(message, response);

            char *response_str = cJSON_PrintUnformatted(response);
            cJSON_Delete(response);
            if (err == ESP_OK) send_message(response_str);
            free(response_str);
            cJSON_Delete(message);

            break;

        case WEBSOCKET_EVENT_ERROR:
            if (VERBOSE) ESP_LOGE(TAG, "WebSocket error occurred");
            break;

        default:
            if (VERBOSE) ESP_LOGI(TAG, "WebSocket event ID: %ld", event_id);
            break;
    }
}

void websocket_task(void *pvParameters) {
    while (true) {
        ws_payload message = {
            .type = 1,
        };

        uint8_t eleven_bytes[11];
        uint8_t two_bytes[2];

        read_bytes(I2C_DATA_SUBCLASS_ID, I2C_NAME_OFFSET, eleven_bytes, sizeof(eleven_bytes));
        if (strcmp((char *)eleven_bytes, "") != 0) strncpy(ESP_ID, (char *)eleven_bytes, 10);
        // cJSON_AddStringToObject(data, "name", ESP_ID);
        char esp_id_number[3];
        strcpy(esp_id_number, &ESP_ID[4]);
        message.esp_id = atoi(esp_id_number);

        // read sensor data
        // these values are big-endian
        read_bytes(0, I2C_STATE_OF_CHARGE_REG, two_bytes, sizeof(two_bytes));
        cJSON_AddNumberToObject(data, "Q", two_bytes[1] << 8 | two_bytes[0]);
        message.Q = (uint8_t)(two_bytes[1] << 8 | two_bytes[0]);

        read_bytes(0, I2C_STATE_OF_HEALTH_REG, two_bytes, sizeof(two_bytes));
        cJSON_AddNumberToObject(data, "H", two_bytes[1] << 8 | two_bytes[0]);
        message.H = (uint8_t)(two_bytes[1] << 8 | two_bytes[0]);

        read_bytes(0, I2C_VOLTAGE_REG, two_bytes, sizeof(two_bytes));
        cJSON_AddNumberToObject(data, "V", round_to_dp((float)(two_bytes[1] << 8 | two_bytes[0]) / 1000.0, 1));
        message.V = (uint8_t)round_to_dp((float)(two_bytes[1] << 8 | two_bytes[0]) / 1000.0, 1);

        read_bytes(0, I2C_CURRENT_REG, two_bytes, sizeof(two_bytes));
        cJSON_AddNumberToObject(data, "I", round_to_dp((float)((int16_t)(two_bytes[1] << 8 | two_bytes[0])) / 1000.0, 1));
        message.I = (int8_t)round_to_dp((float)(two_bytes[1] << 8 | two_bytes[0]) / 1000.0, 1);

        read_bytes(0, I2C_TEMPERATURE_REG, two_bytes, sizeof(two_bytes));
        cJSON_AddNumberToObject(data, "aT", round_to_dp((float)(two_bytes[1] << 8 | two_bytes[0]) / 10.0 - 273.15, 1));
        message.aT = (int16_t)round_to_dp((float)(two_bytes[1] << 8 | two_bytes[0]) / 10.0 - 273.15, 1);

        read_bytes(0, I2C_INT_TEMPERATURE_REG, two_bytes, sizeof(two_bytes));
        cJSON_AddNumberToObject(data, "iT", round_to_dp((float)(two_bytes[1] << 8 | two_bytes[0]) / 10.0 - 273.15, 1));
        message.iT = (int16_t)round_to_dp((float)(two_bytes[1] << 8 | two_bytes[0]) / 10.0 - 273.15, 1);

        // configurable data too
        // these values are little-endian
        read_bytes(I2C_DISCHARGE_SUBCLASS_ID, I2C_BL_OFFSET, two_bytes, sizeof(two_bytes));
        cJSON_AddNumberToObject(data, "BL", two_bytes[0] << 8 | two_bytes[1]);
        message.BL = (uint16_t)(two_bytes[0] << 8 | two_bytes[1]);

        read_bytes(I2C_DISCHARGE_SUBCLASS_ID, I2C_BH_OFFSET, two_bytes, sizeof(two_bytes));
        cJSON_AddNumberToObject(data, "BH", two_bytes[0] << 8 | two_bytes[1]);
        message.BH = (uint16_t)(two_bytes[0] << 8 | two_bytes[1]);

        read_bytes(I2C_CURRENT_THRESHOLDS_SUBCLASS_ID, I2C_CHG_CURRENT_THRESHOLD_OFFSET, two_bytes, sizeof(two_bytes));
        cJSON_AddNumberToObject(data, "CCT", two_bytes[0] << 8 | two_bytes[1]);
        message.CCT = (uint8_t)(two_bytes[0] << 8 | two_bytes[1]);

        read_bytes(I2C_CURRENT_THRESHOLDS_SUBCLASS_ID, I2C_DSG_CURRENT_THRESHOLD_OFFSET, two_bytes, sizeof(two_bytes));
        cJSON_AddNumberToObject(data, "DCT", two_bytes[0] << 8 | two_bytes[1]);
        message.DCT = (uint8_t)(two_bytes[0] << 8 | two_bytes[1]);

        read_bytes(I2C_CHARGE_INHIBIT_CFG_SUBCLASS_ID, I2C_CHG_INHIBIT_TEMP_LOW_OFFSET, two_bytes, sizeof(two_bytes));
        cJSON_AddNumberToObject(data, "CITL", two_bytes[0] << 8 | two_bytes[1]);
        message.CITL = (int8_t)(two_bytes[0] << 8 | two_bytes[1]);

        read_bytes(I2C_CHARGE_INHIBIT_CFG_SUBCLASS_ID, I2C_CHG_INHIBIT_TEMP_HIGH_OFFSET, two_bytes, sizeof(two_bytes));
        cJSON_AddNumberToObject(data, "CITH", two_bytes[0] << 8 | two_bytes[1]);
        message.CITH = (uint16_t)(two_bytes[0] << 8 | two_bytes[1]);

        // get sensor data
        char* data_string = LORA_IS_RECEIVER ? "" : get_data(false); // goes to website

        char* full_data_string = get_data(true); // goes to local clients

        if (full_data_string != NULL && data_string != NULL) {
            // first send to all connected WebSocket clients
            for (int i = 0; i < WS_CONFIG_MAX_CLIENTS; i++) {
                if (client_sockets[i].is_browser_not_mesh && client_sockets[i].descriptor >= 0) {
                    httpd_ws_frame_t ws_pkt = {
                        .payload = (uint8_t *)full_data_string,
                        .len = strlen(full_data_string),
                        .type = HTTPD_WS_TYPE_TEXT,
                    };

                    int tries = 0;
                    int max_tries = 5;
                    esp_err_t err;
                    while (tries < max_tries) {
                        err = httpd_ws_send_frame_async(server, client_sockets[i].descriptor, &ws_pkt);
                        if (err == ESP_OK) break;
                        vTaskDelay(pdMS_TO_TICKS(1000));
                        tries++;
                    }

                    if (err != ESP_OK) {
                        ESP_LOGE(TAG, "Failed to send frame to client %d: %s", client_sockets[i].descriptor, esp_err_to_name(err));
                        remove_client(client_sockets[i].descriptor);  // Clean up disconnected clients
                    }
                }
            }

            // then to website over internet
            if (connected_to_WiFi) {
                // get Wi-Fi station gateway
                esp_netif_t *sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
                if (sta_netif == NULL) return;

                // retrieve the IP information
                esp_netif_ip_info_t ip_info;
                esp_netif_get_ip_info(sta_netif, &ip_info);

                // add correct ESP32 IP info to message
                esp_ip4addr_ntoa(&ip_info.ip, ESP_IP, 16);

                char s[64];
                if (LOCAL) {
                    snprintf(s, sizeof(s), "%s:5000", FLASK_IP);
                } else {
                    snprintf(s, sizeof(s), "%s", AZURE_URL);
                }
                char uri[128];
                snprintf(uri, sizeof(uri), "wss://%s/esp_ws", s);

                const esp_websocket_client_config_t websocket_cfg = {
                    .uri = uri,
                    .reconnect_timeout_ms = 10000,
                    .network_timeout_ms = 10000,
                    .cert_pem = LOCAL?(const char *)local_cert_pem:(const char *)website_cert_pem,
                    .skip_cert_common_name_check = LOCAL,
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
                    vTaskDelay(pdMS_TO_TICKS(5000));
                }


                if (!esp_websocket_client_is_connected(ws_client)) {
                    esp_websocket_client_stop(ws_client);
                    esp_websocket_client_destroy(ws_client);
                    ws_client = NULL;
                } else {
                    char message[WS_MESSAGE_MAX_LEN];
                    snprintf(message, sizeof(message), "{\"type\":\"data\",\"id\":\"%s\",\"content\":%s}", ESP_ID, data_string);
                    send_message(message);
                    // send_message(&message);
                }
            }

            // clean up
            free(data_string);
            free(full_data_string);
        }

        // check wifi connection still exists
        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
            connected_to_WiFi = false;
            if (DEV) send_fake_request();
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
