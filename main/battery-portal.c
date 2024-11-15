#include <esp_spiffs.h>

#include "include/AP.h"
#include "include/DNS.h"

// #include "include/WS.h"
#include <esp_log.h>
#include <esp_http_server.h>
#include <lwip/sockets.h>
#include <string.h>

#include "include/I2C.h"

// httpd_handle_t server;  // Define the server globally
#define CONFIG_MAX_CLIENTS 2
typedef struct {
    int fd;  // File descriptor for the client
    bool active;  // Track if the client is currently active
} ws_client_t;
static ws_client_t client_list[CONFIG_MAX_CLIENTS] = {0};  // Initialize client list

#define MIN(a,b) ((a) < (b) ? (a) : (b))

const char login_html[] = "<html>\n"
    "<body>\n"
    "<h2>Login Page</h2>\n"
    "<form action='/ws' method='post'>\n"
    "<label>Username:</label>\n"
    "<input type='text' name='username' required><br>\n"
    "<label>Password:</label>\n"
    "<input type='password' name='password' required><br>\n"
    "<button type='submit'>Submit</button>\n"
    "</form>\n"
    "</body>\n"
    "</html>";

const char ws_html[] = "<html>\n"
    "<body>\n"
    "<h2>WebSocket Page</h2>\n"
    "<p>Time: <span id='time'></span></p>\n"
    "<div id='content'>Waiting for updates...</div>\n"
    "<script>\n"
    "var ws = new WebSocket('ws://' + window.location.host + '/ws');\n"
    "ws.onmessage = function(event) {\n"
    "    document.getElementById('content').innerText = event.data;\n"
    "    document.getElementById('time').innerHTML = event.data;\n"
    "};\n"
    "ws.onerror = function(error) {\n"
    "  console.error('WebSocket Error:', error);\n"
    "};\n"
    "ws.onclose = function() {\n"
    "  console.log('WebSocket connection closed');\n"
    "};\n"
    "</script>\n"
    "</body>\n"
    "</html>";

static esp_err_t login_get_handler(httpd_req_t *req) {
    httpd_resp_send(req, login_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t ws_post_handler(httpd_req_t *req) {
    ESP_LOGW("HERE", "4");
    char content[100];
    size_t recv_size = MIN(req->content_len, sizeof(content) - 1);

    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    content[recv_size] = '\0';

    if (strstr(content, "username=admin") && strstr(content, "password=1234")) {
        httpd_resp_send(req, ws_html, HTTPD_RESP_USE_STRLEN);
    } else {
        httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Invalid username or password");
    }

    return ESP_OK;
}

static esp_err_t websocket_handler(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        // Add new client to the list
        for (int i = 0; i < CONFIG_MAX_CLIENTS; i++) {
            if (!client_list[i].active) {
                client_list[i].fd = httpd_req_to_sockfd(req);
                client_list[i].active = true;
                break;
            }
        }

        ESP_LOGW("HERE", "3");
        httpd_ws_frame_t ws_frame;
        memset(&ws_frame, 0, sizeof(httpd_ws_frame_t));
        ws_frame.type = HTTPD_WS_TYPE_TEXT;
        ws_frame.payload = (uint8_t *)"<html><body><h1>Welcome to WebSocket Captive Portal</h1></body></html>";
        ws_frame.len = strlen((char *)ws_frame.payload);
        httpd_ws_send_frame(req, &ws_frame);
        return ESP_OK;
    }
    return ESP_FAIL;
}

void websocket_broadcast_task(void *pvParameters) {
    httpd_handle_t *server = (httpd_handle_t *)pvParameters;
    if (*server == NULL) {
        ESP_LOGE("WebSocket", "Server handle is NULL");
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGW("HERE", "1");
    char time_buffer[64];
    while (true) {
        ESP_LOGW("HERE", "2");
        // Get the current time
        time_t now = time(NULL);
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);

        // Format the current time as a string
        strftime(time_buffer, sizeof(time_buffer), "Time: %H:%M:%S (LastSync)", &timeinfo);

        httpd_ws_frame_t ws_frame;
        memset(&ws_frame, 0, sizeof(httpd_ws_frame_t));
        ws_frame.type = HTTPD_WS_TYPE_TEXT;
        ws_frame.payload = (uint8_t *)time_buffer;
        ws_frame.len = strlen((char *)ws_frame.payload);
        
        for (int i = 0; i < CONFIG_MAX_CLIENTS; i++) {
            if (client_list[i].active) {
                if (httpd_ws_send_frame_async(*server, client_list[i].fd, &ws_frame) != ESP_OK) {
                    ESP_LOGE("WS", "Failed to send frame to client %d", client_list[i].fd);
                    client_list[i].active = false;  // Mark client as inactive if sending fails
                } else {
                    ESP_LOGI("WS", "Refreshed websocket %d successfully", i);
                }
            } else {
                ESP_LOGW("WS", "Websocket %d not active", i);
            }
        }

        ESP_LOGI("WS", "Refreshed websockets");

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

httpd_handle_t start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t login_uri = {
            .uri       = "/login",
            .method    = HTTP_GET,
            .handler   = login_get_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &login_uri);
        login_uri.uri = "/hotspot-detect.html";
        httpd_register_uri_handler(server, &login_uri);
        login_uri.uri = "/generate_204";
        httpd_register_uri_handler(server, &login_uri);
        login_uri.uri = "/connecttest.txt";
        httpd_register_uri_handler(server, &login_uri);
        login_uri.uri = "/favicon.ico";
        httpd_register_uri_handler(server, &login_uri);
        login_uri.uri = "/redirect";
        httpd_register_uri_handler(server, &login_uri);

        httpd_uri_t ws_uri = {
            .uri       = "/ws",
            .method    = HTTP_POST,
            .handler   = ws_post_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &ws_uri);

        httpd_uri_t websocket_uri = {
            .uri       = "/ws",
            .method    = HTTP_GET,
            .handler   = websocket_handler,
            .is_websocket = true,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &websocket_uri);
    }
    return server;
}

void app_main(void) {
    /*
    // initialise SPIFFS
    esp_vfs_spiffs_conf_t config = {
        .base_path = "/storage",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };
    esp_err_t result = esp_vfs_spiffs_register(&config);
    if (result != ESP_OK) {
        ESP_LOGE("SPIFFS", "Failed to initialise SPIFFS (%s)", esp_err_to_name(result));
        return;
    }

    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI("main", "I2C initialized successfully");

    uint16_t iCharge = read_2byte_data(STATE_OF_CHARGE_REG);
    ESP_LOGI("main", "Charge: %d %%", iCharge);

    uint16_t iVoltage = read_2byte_data(VOLTAGE_REG);
    float fVoltage = (float)iVoltage / 1000.0;
    ESP_LOGI("main", "Voltage: %.2f V", fVoltage);

    uint16_t iCurrent = read_2byte_data(CURRENT_REG);
    float fCurrent = (float)iCurrent / 1000.0;
    ESP_LOGI("main", "Current: %.2f A", fCurrent);

    uint16_t iTemperature = read_2byte_data(TEMPERATURE_REG);
    float fTemperature = (float)iTemperature / 10.0 - 273.15;
    ESP_LOGI("main", "Temperature: %.2f \u00B0C", fTemperature);
    */

    // Start the Access Point
    wifi_init_softap();

    // Start the ws server to serve the HTML page
    // static httpd_handle_t server = NULL;
    httpd_handle_t server = start_webserver();

    // Start DNS server task
    xTaskCreate(&dns_server_task, "dns_server_task", 4096, NULL, 5, NULL);

    // xTaskCreate(&websocket_send_task, "websocket_send_task", 4096, server, 5, NULL);    // Create a task to periodically send WebSocket updates
    xTaskCreate(&websocket_broadcast_task, "websocket_broadcast_task", 4096, &server, 5, NULL);
}
