#include "WS.h"

#include "BMS.h"
#include "I2C.h"
#include "TASK.h"
#include "config.h"
#include "global.h"
#include "utils.h"

#include "cert.h"
#include "local_cert.h"

#include <inttypes.h>

#include "cJSON.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_websocket_client.h"
#include "esp_wifi.h"
#include "esp_wifi_types_generic.h"
#include "lwip/ip4_addr.h"
#include "nvs_flash.h"

static const char* TAG = "WS";

static bool reconnect = false;
static char ESP_IP[16] = "xxx.xxx.xxx.xxx\0";
static esp_websocket_client_handle_t ws_client = NULL;

static TimerHandle_t websocket_timer;

void add_client(int fd, const char* tkn, bool browser, uint8_t esp_id) {
    for (int i = 0; i < WS_CONFIG_MAX_CLIENTS; i++) {
        if (client_sockets[i].descriptor == fd) {
            return;
        } else if (client_sockets[i].descriptor < 0) {
            client_sockets[i].descriptor = fd;
            strncpy(client_sockets[i].auth_token, tkn, UTILS_AUTH_TOKEN_LENGTH);
            client_sockets[i].auth_token[UTILS_AUTH_TOKEN_LENGTH - 1] = '\0';
            client_sockets[i].is_browser_not_mesh = browser;
            client_sockets[i].esp_id = browser ? 0 : esp_id;
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
            client_sockets[i].esp_id = 0;
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
            uint8_t esp_id = 0;
            if (is_browser_not_mesh) {
                sscanf(check_new_browser_session, "/browser_ws?auth_token=%50s", auth_token);
            } else {
                sscanf(check_new_mesh_session, "/mesh_ws?auth_token=%50[^&]", auth_token);
                char *check_esp_id = strstr(req->uri, "esp_id=");
                if (!check_esp_id) {
                    ESP_LOGE(TAG, "No `esp_id` passed during attempted client WebSocket connection to mesh!");
                    return ESP_FAIL;
                }
                sscanf(check_esp_id, "esp_id=%hhu", &esp_id);
            }
            auth_token[UTILS_AUTH_TOKEN_LENGTH - 1] = '\0';
            if (auth_token[0] != '\0' && strcmp(auth_token, current_auth_token) == 0) {
                add_client(fd, auth_token, is_browser_not_mesh, esp_id);
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
            cJSON *esp_id_obj = cJSON_GetObjectItem(message, "esp_id");
            if (!esp_id_obj) {
                ESP_LOGE(TAG, "incoming LoRa queue message not formatted properly:\n  %s", cJSON_PrintUnformatted(message));
                free(ws_pkt.payload);
                cJSON_Delete(message);
                return ESP_FAIL;
            }
            bool found = false;
            for (int i=0; i<MESH_SIZE; i++) {
                if (all_messages[i].esp_id == esp_id_obj->valueint) {
                    // update existing message
                    found = true;
                    strcpy(all_messages[i].message, (char *)ws_pkt.payload);
                    i = MESH_SIZE;
                } else if (all_messages[i].esp_id == 0) {
                    // create new message
                    found = true;
                    all_messages[i].esp_id = esp_id_obj->valueint;
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
    cJSON_AddNumberToObject(response, "esp_id", ESP_ID);
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
            // xQueueReset(job_queue); // prioritise this reply(?)
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

            cJSON *esp_id = cJSON_GetObjectItem(data, "new_esp_id");
            if (esp_id && esp_id->valueint != 0 && esp_id->valueint != ESP_ID) {
                ESP_LOGI(TAG, "Changing device name...");

                uint8_t address[2] = {0};
                convert_uint_to_n_bytes(I2C_DEVICE_NAME_ADDR, address, sizeof(address), true);

                // construct DeviceName string from ID
                char new_id[7];
                snprintf(new_id, sizeof(new_id), "bms_%02u", esp_id->valueint);
                size_t new_id_length = strlen(new_id);

                // convert to data array
                uint8_t new_id_data[1+new_id_length];
                new_id_data[0] = new_id_length;
                for (size_t i=0; i<new_id_length; i++)
                    new_id_data[1 + i] = new_id[i];

                // write to DataFlash
                write_data_flash(address, sizeof(address), new_id_data, sizeof(new_id_data));

                // update ESP32 memory
                change_esp_id(new_id);
            }

            int OTC = cJSON_GetObjectItem(data, "OTC")->valueint;

            uint8_t address[2] = {0};
            convert_uint_to_n_bytes(I2C_OTC_THRESHOLD_ADDR, address, sizeof(address), true);
            uint8_t data_flash[2] = {0};
            read_data_flash(address, sizeof(address), data_flash, sizeof(data_flash));
            if (OTC != (data_flash[1] << 8 | data_flash[0])) {
                ESP_LOGI(TAG, "Changing OTC threshold...");
                convert_uint_to_n_bytes(OTC, data_flash, sizeof(data_flash), false);
                write_data_flash(address, sizeof(address), data_flash, sizeof(data_flash));
            }

            cJSON_AddStringToObject(response_content, "status", "success");
        } else if (summary && strcmp(summary->valuestring, "connect-wifi") == 0) {
            cJSON *data = cJSON_GetObjectItem(content, "data");
            if (!data) {
                ESP_LOGE(TAG, "Failed to parse JSON");
                cJSON_AddStringToObject(response_content, "status", "error");
                return ESP_FAIL;
            }

            const char* ssid = cJSON_GetObjectItem(data, "ssid")->valuestring;
            const char* password = cJSON_GetObjectItem(data, "password")->valuestring;
            cJSON *auto_connect = cJSON_GetObjectItem(data, "auto_connect");

            int tries = 0;
            int max_tries = 10;
            if (!connected_to_WiFi || reconnect) {
                if (reconnect) {
                    connected_to_WiFi = false;
                }

                wifi_config_t *wifi_sta_config = malloc(sizeof(wifi_config_t));
                memset(wifi_sta_config, 0, sizeof(wifi_config_t));

                // overwrite stored values in NVS
                nvs_handle_t nvs;
                esp_err_t err = nvs_open("WIFI", NVS_READWRITE, &nvs);
                if (err != ESP_OK) {
                    ESP_LOGW(TAG, "Could not open NVS namespace for writing");
                } else {
                    nvs_set_str(nvs, "SSID", ssid);
                    nvs_set_str(nvs, "PASSWORD", password);
                    if (cJSON_IsBool(auto_connect) && cJSON_IsTrue(auto_connect)) nvs_set_u8(nvs, "AUTO_CONNECT", 1);
                    else nvs_set_u8(nvs, "AUTO_CONNECT", 0);
                    nvs_commit(nvs);
                    nvs_close(nvs);
                }

                strncpy((char *)wifi_sta_config->sta.ssid, ssid, sizeof(wifi_sta_config->sta.ssid) - 1);
                if (strlen(password) == 0) {
                    wifi_sta_config->ap.authmode = WIFI_AUTH_OPEN;
                } else {
                    strncpy((char *)wifi_sta_config->sta.password, password, sizeof(wifi_sta_config->sta.password) - 1);
                }
                ESP_LOGI(TAG, "Connecting to AP... SSID: %s", wifi_sta_config->sta.ssid);

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
    if (esp_websocket_client_is_connected(ws_client)) {
        if (VERBOSE) ESP_LOGI(TAG, "Sending: %s", message);
        esp_websocket_client_send_text(ws_client, message, strlen(message), portMAX_DELAY);
    } else {
        ESP_LOGW(TAG, "WebSocket not connected, dropping message: %s", message);
    }
}

void process_event(char* data) {
    cJSON *message = cJSON_Parse(data);
    if (!message) {
        ESP_LOGE(TAG, "Invalid json: \"%s\"", data);
        cJSON_Delete(message);
        return;
    }

    if (LORA_IS_RECEIVER) {
        cJSON* type = cJSON_GetObjectItem(message, "type");
        cJSON* content = cJSON_GetObjectItem(message, "content");
        if (!(type && content)) {
            ESP_LOGE(TAG, "Couldn't parse websocket event");
            return;
        }
        if (strcmp(type->valuestring, "response") == 0 && strcmp(content->valuestring, "OK") == 0) {
            if (VERBOSE) ESP_LOGI(TAG, "Receiver: ignorning acknowledgement response message from web server");
            return;
        } else {
            char* message_string = cJSON_PrintUnformatted(message);
            if (message_string) {
                if (VERBOSE) {
                    ESP_LOGI(TAG, "Receiver: received message from web server:");
                    ESP_LOGI(TAG, "%s", message_string);
                    ESP_LOGI(TAG, "adding to outgoing forwarded radio transmissions...");
                }
                strncpy(forwarded_message, message_string, strlen(message_string));
                free(message_string);
            }
        }
    } else {
        if (VERBOSE) {
            char* message_string = cJSON_PrintUnformatted(message);
            if (message_string) {
                ESP_LOGI(TAG, "WebSocket client: received message from ROOT:");
                ESP_LOGI(TAG, "%s", message_string);
            }
        }

        cJSON *response = cJSON_CreateObject();
        esp_err_t err = perform_request(message, response);

        char *response_str = cJSON_PrintUnformatted(response);
        cJSON_Delete(response);
        if (err == ESP_OK) send_message(response_str);
        free(response_str);
    }
    cJSON_Delete(message);
}

void websocket_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    esp_websocket_event_data_t *ws_event_data = (esp_websocket_event_data_t *)event_data;

    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            if (VERBOSE) ESP_LOGI(TAG, "WebSocket connected");
            break;

        case WEBSOCKET_EVENT_DISCONNECTED:
            if (VERBOSE) ESP_LOGI(TAG, "WebSocket disconnected");
            esp_websocket_client_stop(ws_client);
            break;

        case WEBSOCKET_EVENT_DATA:
            if (ws_event_data->data_len == 0) break;

            char *data = malloc(ws_event_data->data_len + 1);
            if (!data) {
                ESP_LOGE(TAG, "Couldn't assign memory in websocket event handler");
                break;
            }
            memcpy(data, ws_event_data->data_ptr, ws_event_data->data_len);
            data[ws_event_data->data_len] = '\0';

            job_t job = {
                .type = JOB_WS_RECEIVE,
                .data = data,
                .size = ws_event_data->data_len
            };

            if (xQueueSend(job_queue, &job, 0) != pdPASS) {
                if (VERBOSE) ESP_LOGW(TAG, "Queue full, dropping job");
                free(job.data);
            }

            break;

        case WEBSOCKET_EVENT_ERROR:
            if (VERBOSE) ESP_LOGE(TAG, "WebSocket error occurred");
            break;

        default:
            if (VERBOSE) ESP_LOGI(TAG, "WebSocket event ID: %" PRId32, event_id);
            break;
    }
}

char* convert_data_numbers_for_frontend(char* data_string) {
        cJSON *data_json = cJSON_Parse(data_string);
        cJSON *content = cJSON_GetObjectItem(data_json, "content");
        if (content) {
            cJSON *V = cJSON_GetObjectItem(content, "V");
            if (V) cJSON_SetNumberValue(V, (float)V->valueint / 10.0);
            cJSON *I = cJSON_GetObjectItem(content, "I");
            if (I) cJSON_SetNumberValue(I, (float)I->valueint / 10.0);
            cJSON *aT = cJSON_GetObjectItem(content, "aT");
            if (aT) cJSON_SetNumberValue(aT, (float)aT->valueint / 10.0);
            cJSON *cT = cJSON_GetObjectItem(content, "cT");
            if (cT) cJSON_SetNumberValue(cT, (float)cT->valueint / 10.0);

            cJSON *V1 = cJSON_GetObjectItem(content, "V1");
            if (V1) cJSON_SetNumberValue(V1, (float)V1->valueint / 100.0);
            cJSON *V2 = cJSON_GetObjectItem(content, "V2");
            if (V2) cJSON_SetNumberValue(V2, (float)V2->valueint / 100.0);
            cJSON *V3 = cJSON_GetObjectItem(content, "V3");
            if (V3) cJSON_SetNumberValue(V3, (float)V3->valueint / 100.0);
            cJSON *V4 = cJSON_GetObjectItem(content, "V4");
            if (V4) cJSON_SetNumberValue(V4, (float)V4->valueint / 100.0);

            cJSON *I1 = cJSON_GetObjectItem(content, "I1");
            if (I1) cJSON_SetNumberValue(I1, (float)I1->valueint / 100.0);
            cJSON *I2 = cJSON_GetObjectItem(content, "I2");
            if (I2) cJSON_SetNumberValue(I2, (float)I2->valueint / 100.0);
            cJSON *I3 = cJSON_GetObjectItem(content, "I3");
            if (I3) cJSON_SetNumberValue(I3, (float)I3->valueint / 100.0);
            cJSON *I4 = cJSON_GetObjectItem(content, "I4");
            if (I4) cJSON_SetNumberValue(I4, (float)I4->valueint / 100.0);

            cJSON *T1 = cJSON_GetObjectItem(content, "T1");
            if (T1) cJSON_SetNumberValue(T1, (float)T1->valueint / 100.0);
            cJSON *T2 = cJSON_GetObjectItem(content, "T2");
            if (T2) cJSON_SetNumberValue(T2, (float)T2->valueint / 100.0);
            cJSON *T3 = cJSON_GetObjectItem(content, "T3");
            if (T3) cJSON_SetNumberValue(T3, (float)T3->valueint / 100.0);
            cJSON *T4 = cJSON_GetObjectItem(content, "T4");
            if (T4) cJSON_SetNumberValue(T4, (float)T4->valueint / 100.0);
        }
        char* converted_data_string = cJSON_PrintUnformatted(data_json);
        cJSON_Delete(data_json);

        return converted_data_string;
}

void send_websocket_data() {
    // get sensor data
    char* data_string = LORA_IS_RECEIVER ? "" : get_data();
    char* converted_data_string = LORA_IS_RECEIVER ? "" : convert_data_numbers_for_frontend(data_string);

    if (data_string != NULL && converted_data_string != NULL) {
        // first send to all connected WebSocket clients
        for (int i = 0; i < WS_CONFIG_MAX_CLIENTS; i++) {
            if (client_sockets[i].is_browser_not_mesh && client_sockets[i].descriptor >= 0) {
                httpd_ws_frame_t ws_pkt = {
                    .payload = (uint8_t *)converted_data_string,
                    .len = strlen(converted_data_string),
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
                snprintf(s, sizeof(s), "%s:8000", FLASK_IP);
            } else {
                snprintf(s, sizeof(s), "%s", AZURE_URL);
            }
            char uri[128];
            snprintf(uri, sizeof(uri), "wss://%s/api/esp_ws", s);

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
                    return;
                }
                esp_websocket_register_events(ws_client, WEBSOCKET_EVENT_ANY, websocket_event_handler, NULL);
                if (esp_websocket_client_start(ws_client) != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to start WebSocket client");
                    esp_websocket_client_destroy(ws_client);
                    ws_client = NULL;
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    return;
                }
                vTaskDelay(pdMS_TO_TICKS(5000));
            }


            if (!esp_websocket_client_is_connected(ws_client)) {
                esp_websocket_client_stop(ws_client);
                esp_websocket_client_destroy(ws_client);
                ws_client = NULL;
            } else {
                if (!LORA_IS_RECEIVER) {
                    // send in json array so webserver can parse
                    cJSON* json_array = cJSON_CreateArray();
                    if (json_array == NULL) {
                        ESP_LOGE(TAG, "Failed to create JSON array");
                        return;
                    }
                    cJSON *item = cJSON_Parse(data_string);
                    if (!item) {
                        ESP_LOGE(TAG, "Failed to parse JSON");
                        cJSON_Delete(json_array);
                        return;
                    }
                    cJSON_AddItemToArray(json_array, item);
                    send_message(cJSON_PrintUnformatted(json_array));

                    cJSON_Delete(json_array);
                }
            }
        }

        // clean up
        if (!LORA_IS_RECEIVER) {
            free(data_string);
            free(converted_data_string);
        }
    }

    // check wifi connection still exists
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
        connected_to_WiFi = false;

        uint8_t auto_connect = (uint8_t)WIFI_AUTO_CONNECT;
        nvs_handle_t nvs;
        esp_err_t err = nvs_open("WIFI", NVS_READONLY, &nvs);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Could not open NVS namespace");
        } else {
            if (nvs_get_u8(nvs, "AUTO_CONNECT", &auto_connect) != ESP_OK) ESP_LOGW(TAG, "Could not read Wi-Fi auto-connect setting from NVS, using default");
            if (auto_connect != 0) send_fake_request();
            nvs_close(nvs);
        }
    }
}

void websocket_callback(TimerHandle_t xTimer) {
    job_t job = {
        .type = JOB_WS_SEND
    };

    if (xQueueSend(job_queue, &job, 0) != pdPASS)
        if (VERBOSE) ESP_LOGW(TAG, "Queue full, dropping job");
}

void start_websocket_timed_task() {
    websocket_timer = xTimerCreate("websocket_timer", pdMS_TO_TICKS(WS_DELAY), pdTRUE, NULL, websocket_callback);
    assert(websocket_timer);
    xTimerStart(websocket_timer, 0);
}
