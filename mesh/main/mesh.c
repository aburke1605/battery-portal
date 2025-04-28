#include <string.h>
#include <ctype.h>

#include <cJSON.h>
#include <esp_log.h>
#include <esp_err.h>
#include <nvs_flash.h>
#include <esp_wifi.h>
#include <esp_http_server.h>
#include <esp_websocket_client.h>

static const char* TAG = "MESH";
static const char* SSID = "ESP_AP";
httpd_handle_t server = NULL;

typedef struct {
    int descriptor;
} client_socket;
client_socket client_sockets[3];
static esp_websocket_client_handle_t ws_client = NULL;

bool wifi_scan(void) {
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
        return false;
    }
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_num, ap_info));

    bool AP_exists = false;
    for (int i = 0; i < ap_num; i++) {
        if (strcmp((const char*)ap_info[i].ssid, SSID) == 0) AP_exists = true;
    }
    
    free(ap_info);

    return AP_exists;
}

void wifi_init(void) {
    // initialise NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    // initialise the Wi-Fi stack
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // assume only STA at first
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    vTaskDelay(pdMS_TO_TICKS(100));

    // scan for other WiFi APs
    bool AP_exists = wifi_scan();
    if (!AP_exists) {
        // stop WiFi before changing mode
        ESP_ERROR_CHECK(esp_wifi_stop());

        // create the AP network interface
        esp_netif_create_default_wifi_ap();

        // set mode to AP + STA
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

        // configure the AP
        wifi_config_t wifi_ap_config = {
            .ap = {
                .ssid = "",
                .ssid_len = 0,
                .channel = 1,
                .max_connection = 10,
                .authmode = WIFI_AUTH_OPEN,
            },
        };
        strncpy((char *)wifi_ap_config.ap.ssid, SSID, sizeof(wifi_ap_config.ap.ssid) - 1);
        wifi_ap_config.ap.ssid[sizeof(wifi_ap_config.ap.ssid) - 1] = '\0';
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_ap_config));

        // restart WiFi
        ESP_LOGI(TAG, "Starting WiFi AP... SSID: %s", wifi_ap_config.ap.ssid);
        ESP_ERROR_CHECK(esp_wifi_start());
        vTaskDelay(pdMS_TO_TICKS(100));
    }    
}

esp_err_t client_handler(httpd_req_t *req) {
    // register new clients...
    if (req->method == HTTP_GET) {
        if (strcmp(req->uri, "/mesh_ws") == 0) {
            int fd = httpd_req_to_sockfd(req);
            for (int i = 0; i < 3; i++) {
                if (client_sockets[i].descriptor < 0) {
                    client_sockets[i].descriptor = fd;
                    ESP_LOGI(TAG, "Client %d added", fd);
                    ESP_LOGI(TAG, "Completing WebSocket handshake...");
                    return ESP_OK; // WebSocket handshake happens here
                }
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
        ESP_LOGI(TAG, "Received WebSocket message: %s", (char *)ws_pkt.payload);

        // read messaage data
        cJSON *message = cJSON_Parse((char *)ws_pkt.payload);
        if (!message) {
            ESP_LOGE(TAG, "Failed to parse JSON");
            free(ws_pkt.payload);
            return ESP_FAIL;
        }

        cJSON_Delete(message);
    } else {
        ESP_LOGW(TAG, "Received unsupported WebSocket frame type: %d", ws_pkt.type);
    }

    free(ws_pkt.payload);

    return ESP_OK;
}

httpd_handle_t start_websocket_server(void) {
    // create sockets for clients
    for (int i = 0; i < 3; i++) {
        client_sockets[i].descriptor = -1;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);

    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Registering URI handlers");

        httpd_uri_t uri = {
            .uri = "/mesh_ws",
            .method = HTTP_GET,
            .handler = client_handler,
            .is_websocket = true
        };
        httpd_register_uri_handler(server, &uri);
    } else {
        ESP_LOGE(TAG, "Error starting server!");
    }
    return server;
}

void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    esp_websocket_event_data_t *ws_event_data = (esp_websocket_event_data_t *)event_data;

    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            ESP_LOGI(TAG, "WebSocket connected");
            char websocket_connect_message[128];
            snprintf(websocket_connect_message, sizeof(websocket_connect_message), "{\"type\":\"register\",\"id\":\"%s\"}", "bms_01");
            esp_websocket_client_send_text(ws_client, websocket_connect_message, strlen(websocket_connect_message), portMAX_DELAY);
            break;

        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "WebSocket disconnected");
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

            // cJSON *response = cJSON_CreateObject();
            // esp_err_t err = perform_request(message, response);

            // char *response_str = cJSON_PrintUnformatted(response);
            // cJSON_Delete(response);
            // if (err == ESP_OK) send_message(response_str);
            // free(response_str);
            // cJSON_Delete(message);

            break;

        case WEBSOCKET_EVENT_ERROR:
            ESP_LOGE(TAG, "WebSocket error occurred");
            break;

        default:
            ESP_LOGI(TAG, "WebSocket event ID: %ld", event_id);
            break;
    }
}

void websocket_message_task(void *pvParameters) {
    while (true) {
        // create JSON object with sensor data
        cJSON *data = cJSON_CreateObject();

        cJSON_AddNumberToObject(data, "Q", 1);
        cJSON_AddNumberToObject(data, "H", 1);
        cJSON_AddNumberToObject(data, "V", 2);
        cJSON_AddNumberToObject(data, "I", 3);
        cJSON_AddNumberToObject(data, "aT", 4);
        cJSON_AddNumberToObject(data, "iT", 5);

        char *data_string = cJSON_PrintUnformatted(data); // goes to website

        cJSON_Delete(data);

        if (data_string != NULL) {
            const esp_websocket_client_config_t websocket_cfg = {
                .uri = "ws://192.168.4.1:80/mesh_ws",
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
                esp_websocket_register_events(ws_client, WEBSOCKET_EVENT_ANY, event_handler, NULL);
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
                char message[1024];
                snprintf(message, sizeof(message), "{\"type\":\"data\",\"id\":\"%s\",\"content\":%s}", "bms_01", data_string);
                esp_websocket_client_send_text(ws_client, message, strlen(message), portMAX_DELAY);
            }
        }

        // clean up
        free(data_string);
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
}

void app_main(void)
{
    wifi_init();

    wifi_mode_t mode;
    esp_wifi_get_mode(&mode);
    if (mode == WIFI_MODE_STA) {
        wifi_config_t *wifi_sta_config = malloc(sizeof(wifi_config_t));
        memset(wifi_sta_config, 0, sizeof(wifi_config_t));

        strncpy((char *)wifi_sta_config->sta.ssid, SSID, sizeof(wifi_sta_config->sta.ssid) - 1);
        wifi_sta_config->ap.authmode = WIFI_AUTH_OPEN;
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_sta_config));
        ESP_LOGI(TAG, "Connecting to AP... SSID: %s", wifi_sta_config->sta.ssid);

        uint8_t tries = 0;
        uint8_t max_tries = 10;
        while (tries < max_tries) {
            esp_err_t err = esp_wifi_connect();
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "Success, waiting for connection...");
                vTaskDelay(pdMS_TO_TICKS(1000));
                break;
            } else {
                ESP_LOGW(TAG, "Failed to connect: %s. Retrying...", esp_err_to_name(err));
                tries++;
            }
        }

        xTaskCreate(&websocket_message_task, "websocket_message_task", 4096, NULL, 5, NULL);
    } else {
        server = start_websocket_server();
        if (server == NULL) {
            ESP_LOGE(TAG, "Failed to start web server!");
            return;
        }
    }
}
