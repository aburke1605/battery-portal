#include "include/AP.h"
#include "include/DNS.h"

#include <esp_http_server.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <time.h>

httpd_handle_t server = NULL;

// Static HTML content for the login page
const char login_html[] =
    "<!DOCTYPE html>"
    "<html>"
    "<head><title>Login</title></head>"
    "<body>"
    "<h1>Login Page</h1>"
    "<form action='/display' method='get'>"
    "Username: <input type='text' name='username'><br>"
    "Password: <input type='password' name='password'><br>"
    "<button type='submit'>Login</button>"
    "</form>"
    "</body>"
    "</html>";

// Static HTML content for the display page
const char display_html[] =
    "<!DOCTYPE html>"
    "<html>"
    "<head><title>Time Display</title></head>"
    "<body>"
    "<h1>Live Time</h1>"
    "<div id='time'>Connecting...</div>"
    "<script>"
    "let socket = new WebSocket('ws://' + location.host + '/ws');"
    "socket.onopen = function() {"
    "    console.log('WebSocket connection established');"
    "};"
    "socket.onmessage = function(event) {"
    "    document.getElementById('time').textContent = event.data;"
    "};"
    "socket.onerror = function(error) {"
    "    console.error('WebSocket error:', error);"
    "};"
    "socket.onclose = function() {"
    "    console.log('WebSocket connection closed');"
    "};"
    "</script>"
    "</body>"
    "</html>";

// WebSocket clients queue
#define MAX_CLIENTS 10
static int client_sockets[MAX_CLIENTS];
void add_client(int fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] == -1) {
            client_sockets[i] = fd;
            ESP_LOGI("WEBSOCKET", "Client %d added", fd);
            return;
        }
    }
    ESP_LOGE("WEBSOCKET", "No space for client %d", fd);
}
void remove_client(int fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] == fd) {
            client_sockets[i] = -1;
            ESP_LOGI("WEBSOCKET", "Client %d removed", fd);
            return;
        }
    }
}

// HTTP handler for the login page
esp_err_t login_get_handler(httpd_req_t *req) {
    httpd_resp_send(req, login_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// HTTP handler for the display page
esp_err_t display_get_handler(httpd_req_t *req) {
    httpd_resp_send(req, display_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// WebSocket handler
esp_err_t websocket_handler(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        int fd = httpd_req_to_sockfd(req);
        ESP_LOGI("WEBSOCKET", "WebSocket handshake complete for client %d", fd);
        add_client(fd);
        return ESP_OK;  // WebSocket handshake happens here
    }

    return ESP_FAIL;
}

// Start the HTTP server
httpd_handle_t start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t login_uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = login_get_handler,
            .user_ctx = NULL
        };
        httpd_uri_t display_uri = {
            .uri = "/display",
            .method = HTTP_GET,
            .handler = display_get_handler,
            .user_ctx = NULL
        };
        httpd_uri_t ws_uri = {
            .uri = "/ws",
            .method = HTTP_GET,
            .handler = websocket_handler,
            .is_websocket = true
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
        httpd_register_uri_handler(server, &display_uri);
        httpd_register_uri_handler(server, &ws_uri);
    }

    return server;
}

// Broadcast task to send live time
void websocket_broadcast_task(void *pvParameters) {
    char buffer[32];
    // int client_fd;

    while (1) {
        time_t now = time(NULL);
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);
        strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);

        // Log the current time
        ESP_LOGI("WEBSOCKET", "Broadcasting time: %s", buffer);

        // Send the time to all connected WebSocket clients
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] != -1) {
                ESP_LOGI("WEBSOCKET", "Attempting to send frame to client %d", client_sockets[i]);
                
                // Validate WebSocket connection with a PING
                esp_err_t ping_status = httpd_ws_send_frame(server, client_sockets[i], &(httpd_ws_frame_t){
                    .payload = NULL,
                    .len = 0,
                    .type = HTTPD_WS_TYPE_PING
                });

                if (ping_status != ESP_OK) {
                    ESP_LOGE("WEBSOCKET", "Client %d disconnected. Removing.", client_sockets[i]);
                    remove_client(client_sockets[i]);
                    continue;
                }

                httpd_ws_frame_t ws_pkt = {
                    .payload = (uint8_t *)buffer,
                    .len = strlen(buffer),
                    .type = HTTPD_WS_TYPE_TEXT,
                    // .final = true,                 // Mark the frame as the final fragment
                };
                esp_err_t err = httpd_ws_send_frame_async(server, client_sockets[i], &ws_pkt);
                if (err != ESP_OK) {
                    ESP_LOGE("WEBSOCKET", "Failed to send frame to client %d: %s", client_sockets[i], esp_err_to_name(err));
                    remove_client(client_sockets[i]);  // Clean up disconnected clients
                } else {
                    ESP_LOGI("WEBSOCKET", "Frame sent to client %d", client_sockets[i]);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));  // Update every second
    }
}

// Main application entry point
void app_main() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = -1;
    }

    wifi_init_softap();  // Assume this initializes the Wi-Fi in SoftAP mode
    server = start_webserver();   // Start the HTTP server
    if (server == NULL) {
        ESP_LOGE("MAIN", "Failed to start web server!");
        return;
    }

    xTaskCreate(&dns_server_task, "dns_server_task", 4096, NULL, 5, NULL);

    xTaskCreate(&websocket_broadcast_task, "websocket_broadcast_task", 4096, NULL, 5, NULL);
}
