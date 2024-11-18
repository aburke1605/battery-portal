#include "include/WS.h"

#include "html/login_page.h"
#include "html/display_page.h"
#include "html/about_page.h"
#include "html/device_page.h"


void add_client(int fd) {
    for (int i = 0; i < CONFIG_MAX_CLIENTS; i++) {
        if (client_sockets[i] == -1) {
            client_sockets[i] = fd;
            ESP_LOGI("WS", "Client %d added", fd);
            return;
        }
    }
    ESP_LOGE("WS", "No space for client %d", fd);
}

void remove_client(int fd) {
    for (int i = 0; i < CONFIG_MAX_CLIENTS; i++) {
        if (client_sockets[i] == fd) {
            client_sockets[i] = -1;
            ESP_LOGI("WS", "Client %d removed", fd);
            return;
        }
    }
}

esp_err_t login_handler(httpd_req_t *req) {
    httpd_resp_send(req, login_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t display_handler(httpd_req_t *req) {
    httpd_resp_send(req, display_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t websocket_handler(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        int fd = httpd_req_to_sockfd(req);
        ESP_LOGI("WS", "WebSocket handshake complete for client %d", fd);
        add_client(fd);
        return ESP_OK;  // WebSocket handshake happens here
    }

    return ESP_FAIL;
}

esp_err_t about_handler(httpd_req_t *req) {
    httpd_resp_send(req, about_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t device_handler(httpd_req_t *req) {
    httpd_resp_send(req, device_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t image_handler(httpd_req_t *req) {
    // Path to the file in the SPIFFS partition
    const char *file_path = (const char *)req->user_ctx;
    FILE *file = fopen(file_path, "r");

    if (file == NULL) {
        ESP_LOGE("WS", "Failed to open file: %s", file_path);
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    // Set the response type to indicate it's an image
    httpd_resp_set_type(req, "image/png");

    // Buffer for reading file chunks
    char buffer[1024];
    size_t read_bytes;

    // Read the file and send chunks
    while ((read_bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (httpd_resp_send_chunk(req, buffer, read_bytes) != ESP_OK) {
            fclose(file);
            ESP_LOGE("WS", "Failed to send file chunk");
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
    }

    // Close the file and send the final empty chunk to signal the end of the response
    fclose(file);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

// Start HTTP server
httpd_handle_t start_webserver(void) {
    // create sockets for clients
    for (int i = 0; i < CONFIG_MAX_CLIENTS; i++) {
        client_sockets[i] = -1;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.max_uri_handlers = 16; // Increase this number as needed

    // Start the httpd server
    ESP_LOGI("WS", "Starting server on port: '%d'", config.server_port);

    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI("WS", "Registering URI handlers");

        // Login page
        httpd_uri_t login_uri = {
            .uri       = "/",
            .method    = HTTP_GET,
            .handler   = login_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &login_uri);
        
        // do some more to catch all device types
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

        // Display page
        httpd_uri_t display_uri = {
            .uri       = "/display",
            .method    = HTTP_GET,
            .handler   = display_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &display_uri);

        // WebSocket
        httpd_uri_t ws_uri = {
            .uri = "/ws",
            .method = HTTP_GET,
            .handler = websocket_handler,
            .is_websocket = true
        };
        httpd_register_uri_handler(server, &ws_uri);

        // About page
        httpd_uri_t about_uri = {
            .uri       = "/about",
            .method    = HTTP_GET,
            .handler   = about_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &about_uri);

        // Device page
        httpd_uri_t device_uri = {
            .uri       = "/device",
            .method    = HTTP_GET,
            .handler   = device_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &device_uri);

        // Add a handler for serving the image
        httpd_uri_t image_uri = {
            .uri       = "/image/aceon.png",
            .method    = HTTP_GET,
            .handler   = image_handler, // Function to read and send the image
            .user_ctx  = "/storage/aceon.png" // File path as user context
        };
        httpd_register_uri_handler(server, &image_uri);

        httpd_uri_t image_uri_2 = {
            .uri       = "/image/aceon2.png",
            .method    = HTTP_GET,
            .handler   = image_handler, // Function to read and send the image
            .user_ctx  = "/storage/aceon2.png" // File path as user context
        };
        httpd_register_uri_handler(server, &image_uri_2);
    } else {
        ESP_LOGE("WS", "Error starting server!");
    }
    return server;
}



void websocket_broadcast_task(void *pvParameters) {
    char buffer[32];
    // int client_fd;

    while (true) {
        time_t now = time(NULL);
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);
        strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);

        // Log the current time
        ESP_LOGI("WS", "Broadcasting time: %s", buffer);

        // Send the time to all connected WebSocket clients
        for (int i = 0; i < CONFIG_MAX_CLIENTS; i++) {
            if (client_sockets[i] != -1) {
                ESP_LOGI("WS", "Attempting to send frame to client %d", client_sockets[i]);
                /*
                // Validate WebSocket connection with a PING
                esp_err_t ping_status = httpd_ws_send_frame_async(server, client_sockets[i], &(httpd_ws_frame_t){
                    .payload = NULL,
                    .len = 0,
                    .type = HTTPD_WS_TYPE_PING
                });
                ESP_LOGE("WS", "ping error: %s", esp_err_to_name(ping_status));

                if (ping_status != ESP_OK) {
                    ESP_LOGE("WS", "Client %d disconnected. Removing.", client_sockets[i]);
                    remove_client(client_sockets[i]);
                    continue;
                }
                */

                httpd_ws_frame_t ws_pkt = {
                    .payload = (uint8_t *)buffer,
                    .len = strlen(buffer),
                    .type = HTTPD_WS_TYPE_TEXT,
                    // .final = true,                 // Mark the frame as the final fragment
                };
                esp_err_t err = httpd_ws_send_frame_async(server, client_sockets[i], &ws_pkt);
                if (err != ESP_OK) {
                    ESP_LOGE("WS", "Failed to send frame to client %d: %s", client_sockets[i], esp_err_to_name(err));
                    remove_client(client_sockets[i]);  // Clean up disconnected clients
                } else {
                    ESP_LOGI("WS", "Frame sent to client %d", client_sockets[i]);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));  // Update every second
    }
}