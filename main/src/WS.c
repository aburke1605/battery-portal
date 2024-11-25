#include "include/WS.h"
#include "include/I2C.h"

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
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, login_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t display_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
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
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, about_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t device_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, device_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t toggle_handler(httpd_req_t *req) {
    static bool led_on = false;
    led_on = !led_on;
    gpio_set_level(LED_GPIO_PIN, led_on ? 1 : 0);
    ESP_LOGI("WS", "LED is now %s", led_on ? "ON" : "OFF");
    httpd_resp_send(req, "LED Toggled", HTTPD_RESP_USE_STRLEN);
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
        display_uri.uri = "/return";
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

        httpd_uri_t toggle_uri = {
            .uri = "/toggle",
            .method = HTTP_GET,
            .handler = toggle_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &toggle_uri);

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
    // QueueHandle_t *queue = (QueueHandle_t *)pvParameters;
    // char *received_data = NULL; // To hold data received from the queue

    while (true) {
        // get the battery info
        uint16_t iCharge = read_2byte_data(STATE_OF_CHARGE_REG);

        uint16_t iVoltage = read_2byte_data(VOLTAGE_REG);
        float fVoltage = (float)iVoltage / 1000.0;

        uint16_t iCurrent = read_2byte_data(CURRENT_REG);
        float fCurrent = (float)iCurrent / 1000.0;
        if (fCurrent<65.536 && fCurrent>32.767) fCurrent = 65.536 - fCurrent;

        uint16_t iTemperature = read_2byte_data(TEMPERATURE_REG);
        float fTemperature = (float)iTemperature / 10.0 - 273.15;

        cJSON *json = cJSON_CreateObject();
        cJSON_AddNumberToObject(json, "humidity", 60.0);
        cJSON_AddNumberToObject(json, "charge", iCharge);
        cJSON_AddNumberToObject(json, "voltage", fVoltage);
        cJSON_AddNumberToObject(json, "current", fCurrent);
        cJSON_AddNumberToObject(json, "temperature", fTemperature);
        // Check if there is remote data in the queue
        if (xQueueReceive(broadcast_queue, &received_data, 0) == pdPASS) {
            cJSON *remote_json = cJSON_Parse(received_data);
            if (remote_json != NULL) {
                // Add remote data as a nested JSON object
                printf("here!\n");
                if (global_json) {
                    // remove it
                    char *global_string = cJSON_PrintUnformatted(global_json);
                    ESP_LOGE("WS", "global_json: %.*s", strlen(global_string), (char *)global_string);
                    cJSON *remote_data = cJSON_DetachItemFromObject(global_json, "remote_data");
                }
                cJSON_AddItemToObject(json, "remote_data", remote_json);
            } else {
                ESP_LOGW("WS", "Failed to parse remote data JSON");
            }
            free(received_data); // Free received data after parsing
        }

        // Add remote sensor data
        xSemaphoreTake(remote_json_mutex, portMAX_DELAY);
        if (strlen(remote_json) > 0) {
            cJSON *remote_json_obj = cJSON_Parse(remote_json);
            if (remote_json_obj != NULL) {
                // Extract only the relevant fields from remote_json_obj to prevent nesting
                cJSON *remote_humidity = cJSON_GetObjectItem(remote_json_obj, "humidity");
                if (remote_humidity != NULL && cJSON_IsNumber(remote_humidity)) {
                    // Create a clean object for "remote_sensor"
                    cJSON *remote_sensor_clean = cJSON_CreateObject();
                    cJSON_AddNumberToObject(remote_sensor_clean, "humidity", remote_humidity->valuedouble);
                    cJSON_AddItemToObject(json, "remote_sensor", remote_sensor_clean);
                } else {
                    cJSON_AddStringToObject(json, "remote_sensor", "Invalid remote data");
                }
                cJSON_Delete(remote_json_obj); // Clean up parsed remote JSON
            } else {
                cJSON_AddStringToObject(json, "remote_sensor", "Invalid JSON");
            }
        } else {
            cJSON_AddStringToObject(json, "remote_sensor", "No data");
        }
        xSemaphoreGive(remote_json_mutex);

        char *json_string = cJSON_PrintUnformatted(json);
        ESP_LOGI("WS", "Sending WebSocket data: %.*s", strlen(json_string), (char *)json_string);
        cJSON_Delete(json);

        if (json_string != NULL) {
            // Send the JSON data to all connected WebSocket clients
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
                        .payload = (uint8_t *)json_string,
                        .len = strlen(json_string),
                        .type = HTTPD_WS_TYPE_TEXT,
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
            free(json_string);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));  // Update every second
    }
}