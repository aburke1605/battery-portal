#include <string.h>
#include <ctype.h>

#include <cJSON.h>
#include <esp_log.h>
#include <esp_err.h>
#include <nvs_flash.h>
#include <esp_wifi.h>
#include <esp_http_server.h>

static const char* TAG = "MESH";
static const char* SSID = "ESP_AP";
httpd_handle_t server = NULL;

typedef struct {
    int descriptor;
} client_socket;
client_socket client_sockets[3];

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
        // change Wi-Fi mode to both AP and STA
        ESP_ERROR_CHECK(esp_wifi_stop());
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

        // configure the AP
        wifi_config_t wifi_ap_config = {
            .ap = {
                .channel = 1,
                .max_connection = 10,
                .authmode = WIFI_AUTH_OPEN,
            },
        };
        strncpy((char *)wifi_ap_config.ap.ssid, SSID, sizeof(wifi_ap_config.ap.ssid) - 1);
        wifi_ap_config.ap.ssid[sizeof(wifi_ap_config.ap.ssid) - 1] = '\0';
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_ap_config));

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
    } else {
        server = start_websocket_server();
        if (server == NULL) {
            ESP_LOGE(TAG, "Failed to start web server!");
            return;
        }
    }
}
