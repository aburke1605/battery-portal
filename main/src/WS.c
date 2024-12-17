#include <esp_wifi.h>
#include <esp_http_client.h>

#include "include/WS.h"
#include "include/I2C.h"
#include "include/utils.h"


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

esp_err_t validate_login_handler(httpd_req_t *req) {
    char content[100];
    esp_err_t err = get_POST_data(req, content, sizeof(content));

    char username[50] = {0};
    char password[50] = {0};
    // a correct login gets POSTed as:
    //    username=admin&password=1234
    sscanf(content, "username=%49[^&]&password=%49s", username, password);
    // %49:   read up to 49 characters (including the null terminator) to prevent buffer overflow
    // [^&]:  a scan set that matches any character except &
    // s:     reads a sequence of non-whitespace characters until a space, newline, or null terminator is encountered

    if (strcmp(username, "admin") == 0 && strcmp(password, "1234") == 0) {
        // credentials correct
        httpd_resp_set_status(req, "302 Found");
        httpd_resp_set_hdr(req, "Location", "/display"); // redirect to /display
        httpd_resp_send(req, NULL, 0); // no response body
    } else {
        // credentials incorrect
        const char *error_msg = "Invalid username or password.";
        httpd_resp_send(req, error_msg, strlen(error_msg));
    }
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

esp_err_t validate_change_handler(httpd_req_t *req) {
    char content[500];
    esp_err_t err = get_POST_data(req, content, sizeof(content));

    // Check if each parameter exists and parse it
    char *BL_start = strstr(content, "BL_voltage_threshold=");
    char *BH_start = strstr(content, "BH_voltage_threshold=");
    char *CCT_start = strstr(content, "charge_current_threshold=");
    char *DCT_start = strstr(content, "discharge_current_threshold=");
    char *CITL_start = strstr(content, "chg_inhibit_temp_low=");
    char *CITH_start = strstr(content, "chg_inhibit_temp_high=");

    if (BL_start) {
        char BL_voltage_threshold[50] = {0};
        sscanf(BL_start, "BL_voltage_threshold=%49[^&]", BL_voltage_threshold);
        if (BL_voltage_threshold[0] != '\0') {
            ESP_LOGI("I2C", "Changing BL voltage...\n");
            set_I2_value(DISCHARGE_SUBCLASS_ID, BL_OFFSET, atoi(BL_voltage_threshold));
        }
    }

    if (BH_start) {
        char BH_voltage_threshold[50] = {0};
        sscanf(BH_start, "BH_voltage_threshold=%49[^&]", BH_voltage_threshold);
        if (BH_voltage_threshold[0] != '\0') {
            ESP_LOGI("I2C", "Changing BH voltage...\n");
            set_I2_value(DISCHARGE_SUBCLASS_ID, BH_OFFSET, atoi(BH_voltage_threshold));
        }
    }

    if (CCT_start) {
        char charge_current_threshold[50] = {0};
        sscanf(CCT_start, "charge_current_threshold=%49[^&]", charge_current_threshold);
        if (charge_current_threshold[0] != '\0') {
            ESP_LOGI("I2C", "Changing charge current threshold...\n");
            set_I2_value(CURRENT_THRESHOLDS_SUBCLASS_ID, CHG_CURRENT_THRESHOLD_OFFSET, atoi(charge_current_threshold));
        }
    }

    if (DCT_start) {
        char discharge_current_threshold[50] = {0};
        sscanf(DCT_start, "discharge_current_threshold=%49[^&]", discharge_current_threshold);
        if (discharge_current_threshold[0] != '\0') {
            ESP_LOGI("I2C", "Changing discharge current threshold...\n");
            set_I2_value(CURRENT_THRESHOLDS_SUBCLASS_ID, DSG_CURRENT_THRESHOLD_OFFSET, atoi(discharge_current_threshold));
        }
    }

    if (CITL_start) {
        char chg_inhibit_temp_low[50] = {0};
        sscanf(CITL_start, "chg_inhibit_temp_low=%49[^&]", chg_inhibit_temp_low);
        if (chg_inhibit_temp_low[0] != '\0') {
            ESP_LOGI("I2C", "Changing charge inhibit low temperature threshold...\n");
            set_I2_value(CHARGE_INHIBIT_CFG_SUBCLASS_ID, CHG_INHIBIT_TEMP_LOW_OFFSET, atoi(chg_inhibit_temp_low));
        }
    }

    if (CITH_start) {
        char chg_inhibit_temp_high[50] = {0};
        sscanf(CITH_start, "chg_inhibit_temp_high=%49s", chg_inhibit_temp_high);
        if (chg_inhibit_temp_high[0] != '\0') {
            ESP_LOGI("I2C", "Changing charge inhibit high temperature threshold...\n");
            set_I2_value(CHARGE_INHIBIT_CFG_SUBCLASS_ID, CHG_INHIBIT_TEMP_HIGH_OFFSET, atoi(chg_inhibit_temp_high));
        }
    }

    return ESP_OK;
}

esp_err_t validate_connect_handler(httpd_req_t *req) {
    char content[100];
    esp_err_t err = get_POST_data(req, content, sizeof(content));

    char ssid_encoded[50] = {0};
    char ssid[50] = {0};
    char password_encoded[50] = {0};
    char password[50] = {0};
    sscanf(content, "ssid=%49[^&]&password=%49s", ssid_encoded, password_encoded);
    url_decode(ssid, ssid_encoded);
    url_decode(password, password_encoded);

    wifi_config_t wifi_sta_config = {
        .sta = {},
    };
    strncpy((char *)wifi_sta_config.sta.ssid, ssid, sizeof(wifi_sta_config.sta.ssid) - 1);
    strncpy((char *)wifi_sta_config.sta.password, password, sizeof(wifi_sta_config.sta.password) - 1);
    wifi_sta_config.sta.ssid[sizeof(wifi_sta_config.sta.ssid) - 1] = '\0';
    wifi_sta_config.sta.password[sizeof(wifi_sta_config.sta.password) - 1] = '\0';

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_sta_config));

    ESP_LOGI("AP", "Connecting to AP... SSID: %s", wifi_sta_config.sta.ssid);

    // Wait for connection
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(3000)); // 3s delay between attempts
        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            ESP_LOGI("WS", "Connected to router. Signal strength: %d dBm", ap_info.rssi);
            connected_to_WiFi = true;
            break;
        } else {
            ESP_LOGI("WS", "Not connected. Retrying...");
            esp_wifi_connect();
        }
    }

    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/display"); // redirect back to /display
    httpd_resp_send(req, NULL, 0); // no response body

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

esp_err_t css_handler(httpd_req_t *req) {
    const char *file_path = (const char *)req->user_ctx;

    FILE *file = fopen(file_path, "r");
    if (!file) {
        ESP_LOGE("CSS_HANDLER", "Failed to open file: %s", file_path);
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "text/css");

    char buffer[1024];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (httpd_resp_send_chunk(req, buffer, bytes_read) != ESP_OK) {
            fclose(file);
            ESP_LOGE("CSS_HANDLER", "Failed to send chunk");
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
            return ESP_FAIL;
        }
    }

    fclose(file);

    httpd_resp_send_chunk(req, NULL, 0);

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

esp_err_t file_serve_handler(httpd_req_t *req) {
    const char *file_path = (const char *)req->user_ctx; // Get file path from user_ctx
    ESP_LOGI("WS", "Serving file: %s", file_path);

    // Open the file
    FILE *file = fopen(file_path, "r");
    if (!file) {
        ESP_LOGE("WS", "Failed to open file: %s", file_path);
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found");
        return ESP_FAIL;
    }

    // Get file size
    struct stat file_stat;
    if (stat(file_path, &file_stat) != 0) {
        ESP_LOGE("WS", "Failed to get file stats: %s", file_path);
        fclose(file);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to retrieve file size");
        return ESP_FAIL;
    }

    // Set Content-Type based on file extension
    const char *ext = strrchr(file_path, '.');
    if (ext) {
        if (strcmp(ext, ".html") == 0) {
            httpd_resp_set_type(req, "text/html");
        } else if (strcmp(ext, ".css") == 0) {
            httpd_resp_set_type(req, "text/css");
        } else if (strcmp(ext, ".js") == 0) {
            httpd_resp_set_type(req, "application/javascript");
        } else if (strcmp(ext, ".png") == 0) {
            httpd_resp_set_type(req, "image/png");
        } else if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) {
            httpd_resp_set_type(req, "image/jpeg");
        } else if (strcmp(ext, ".ico") == 0) {
            httpd_resp_set_type(req, "image/x-icon");
        } else {
            httpd_resp_set_type(req, "application/octet-stream");
        }
    } else {
        httpd_resp_set_type(req, "application/octet-stream");
    }

    // Buffer to read file contents
    char buffer[1024];
    size_t read_bytes;

    // Send file data in chunks
    while ((read_bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        esp_err_t ret = httpd_resp_send_chunk(req, buffer, read_bytes);
        if (ret != ESP_OK) {
            ESP_LOGE("WS", "Failed to send file chunk");
            fclose(file);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
            return ESP_FAIL;
        }
    }

    // End the HTTP response
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
    config.max_uri_handlers = 32; // Increase this number as needed

    // Start the httpd server
    ESP_LOGI("WS", "Starting server on port: '%d'", config.server_port);

    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI("WS", "Registering URI handlers");

        // Login page
        httpd_uri_t login_uri = {
            .uri       = "/",
            .method    = HTTP_GET,
            .handler   = file_serve_handler,
            .user_ctx  = "/storage/login.html"
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

        // Validate login
        httpd_uri_t validate_login_uri = {
            .uri       = "/validate_login",
            .method    = HTTP_POST,
            .handler   = validate_login_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &validate_login_uri);

        // Display page
        httpd_uri_t display_uri = {
            .uri       = "/display",
            .method    = HTTP_GET,
            .handler   = file_serve_handler,
            .user_ctx  = "/storage/display.html"
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
        
        httpd_uri_t change_uri = {
            .uri       = "/change",
            .method    = HTTP_GET,
            .handler   = file_serve_handler,
            .user_ctx  = "/storage/change.html"
        };
        httpd_register_uri_handler(server, &change_uri);

        httpd_uri_t validate_change_uri = {
            .uri       = "/validate_change",
            .method    = HTTP_POST,
            .handler   = validate_change_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &validate_change_uri);

        // Connect page
        httpd_uri_t connect_uri = {
            .uri       = "/connect",
            .method    = HTTP_GET,
            .handler   = file_serve_handler,
            .user_ctx  = "/storage/connect.html"
        };
        httpd_register_uri_handler(server, &connect_uri);

        // Validate connect
        httpd_uri_t validate_connect_uri = {
            .uri       = "/validate_connect",
            .method    = HTTP_POST,
            .handler   = validate_connect_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &validate_connect_uri);

        // Nearby page
        httpd_uri_t nearby_uri = {
            .uri       = "/nearby",
            .method    = HTTP_GET,
            .handler   = file_serve_handler,
            .user_ctx  = "/storage/nearby.html"
        };
        httpd_register_uri_handler(server, &nearby_uri);

        // About page
        httpd_uri_t about_uri = {
            .uri       = "/about",
            .method    = HTTP_GET,
            .handler   = file_serve_handler,
            .user_ctx  = "/storage/about.html"
        };
        httpd_register_uri_handler(server, &about_uri);

        // Device page
        httpd_uri_t device_uri = {
            .uri       = "/device",
            .method    = HTTP_GET,
            .handler   = file_serve_handler,
            .user_ctx  = "/storage/device.html"
        };
        httpd_register_uri_handler(server, &device_uri);

        httpd_uri_t toggle_uri = {
            .uri = "/toggle",
            .method = HTTP_GET,
            .handler = toggle_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &toggle_uri);

        httpd_uri_t css_uri = {
            .uri      = "/static/style.css",
            .method   = HTTP_GET,
            .handler  = css_handler,
            .user_ctx = "/storage/style.css",
        };
        httpd_register_uri_handler(server, &css_uri);

        // Add a handler for serving the image
        httpd_uri_t image_uri = {
            .uri       = "/static/images/aceon.png",
            .method    = HTTP_GET,
            .handler   = image_handler, // Function to read and send the image
            .user_ctx  = "/storage/images/aceon.png" // File path as user context
        };
        httpd_register_uri_handler(server, &image_uri);

        httpd_uri_t image_uri_2 = {
            .uri       = "/static/images/aceon2.png",
            .method    = HTTP_GET,
            .handler   = image_handler, // Function to read and send the image
            .user_ctx  = "/storage/images/aceon2.png" // File path as user context
        };
        httpd_register_uri_handler(server, &image_uri_2);

    } else {
        ESP_LOGE("WS", "Error starting server!");
    }
    return server;
}



void web_task(void *pvParameters) {
    while (true) {

        // read sensor data
        uint16_t iCharge = read_2byte_data(STATE_OF_CHARGE_REG);
        uint16_t iVoltage = read_2byte_data(VOLTAGE_REG);
        float fVoltage = (float)iVoltage / 1000.0;
        uint16_t iCurrent = read_2byte_data(CURRENT_REG);
        float fCurrent = (float)iCurrent / 1000.0;
        if (fCurrent < 65.536 && fCurrent > 32.767) fCurrent = 65.536 - fCurrent; // this is something to do with 16 bit binary
        uint16_t iTemperature = read_2byte_data(TEMPERATURE_REG);
        float fTemperature = (float)iTemperature / 10.0 - 273.15;

        // configurable data too
        uint16_t iBL = test_read(DISCHARGE_SUBCLASS_ID, BL_OFFSET);
        uint16_t iBH = test_read(DISCHARGE_SUBCLASS_ID, BH_OFFSET);
        uint16_t iCCT = test_read(CURRENT_THRESHOLDS_SUBCLASS_ID, CHG_CURRENT_THRESHOLD_OFFSET);
        uint16_t iDCT = test_read(CURRENT_THRESHOLDS_SUBCLASS_ID, DSG_CURRENT_THRESHOLD_OFFSET);
        uint16_t iCITL = test_read(CHARGE_INHIBIT_CFG_SUBCLASS_ID, CHG_INHIBIT_TEMP_LOW_OFFSET);
        uint16_t iCITH = test_read(CHARGE_INHIBIT_CFG_SUBCLASS_ID, CHG_INHIBIT_TEMP_HIGH_OFFSET);

        // create JSON object with sensor data
        cJSON *json = cJSON_CreateObject();
        cJSON_AddNumberToObject(json, "charge", iCharge);
        cJSON_AddNumberToObject(json, "voltage", fVoltage);
        cJSON_AddNumberToObject(json, "current", fCurrent);
        cJSON_AddNumberToObject(json, "temperature", fTemperature);
        cJSON_AddNumberToObject(json, "BL", iBL);
        cJSON_AddNumberToObject(json, "BH", iBH);
        cJSON_AddNumberToObject(json, "CCT", iCCT);
        cJSON_AddNumberToObject(json, "DCT", iDCT);
        cJSON_AddNumberToObject(json, "CITL", iCITL);
        cJSON_AddNumberToObject(json, "CITH", iCITH);


        // add data received from other ESP32s if available
        if (xSemaphoreTake(data_mutex, portMAX_DELAY)) {
            if (strlen(received_data) > 0) {
                cJSON *received_json = cJSON_Parse(received_data);
                if (received_json != NULL) {
                    // TODO: is the below line actually enough,
                    //       or do we need all of the lines below?
                    // cJSON_AddItemToObject(json, "received_data", received_json);

                    // Create a clean / empty object for "received_data"
                    cJSON *received_data_clean = cJSON_CreateObject();
                    // Iterate through all keys in the parsed JSON
                    cJSON *item = received_json->child;
                    while (item != NULL) {
                        if (cJSON_IsNumber(item)) {
                            cJSON_AddNumberToObject(received_data_clean, item->string, item->valuedouble);
                        } else {
                            ESP_LOGW("WS", "Support for reading %s from server is not implemented", item->string);
                        }
                        item = item->next;
                    }
                    cJSON_AddItemToObject(json, "received_data", received_data_clean);
                    cJSON_Delete(received_json); // Clean up parsed JSON
                } else {
                    ESP_LOGE("WS", "Failed to parse received_data as JSON");
                }
            } else {
                cJSON_AddStringToObject(json, "received_data", "No data");
            }
            xSemaphoreGive(data_mutex);
        } else {
            ESP_LOGE("WS", "Failed to take mutex for broadcast data");
        }


        // now process the json
        char *json_string = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
        if (json_string != NULL) {


            // first send to all connected WebSocket clients
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


            // then send to website over internet
            // but first check if connected to an AP
            if (connected_to_WiFi) {
                esp_http_client_config_t config = {
                    .url = "http://192.168.137.1:5000/data", // this is the IPv4 address of the PC hotspot
                };
                esp_http_client_handle_t client = esp_http_client_init(&config);

                // configure HTTP client for POST
                esp_http_client_set_method(client, HTTP_METHOD_POST);
                esp_http_client_set_header(client, "Content-Type", "application/json");
                esp_http_client_set_post_field(client, json_string, strlen(json_string));

                // perform HTTP POST request
                esp_err_t err = esp_http_client_perform(client);
                if (err == ESP_OK) {
                    int status_code = esp_http_client_get_status_code(client);
                    char response_buf[200];
                    int content_length = esp_http_client_read_response(client, response_buf, sizeof(response_buf) - 1);
                    if (content_length >= 0) {
                        response_buf[content_length] = '\0';
                        ESP_LOGI("main", "HTTP POST Status = %d, Response = %s", status_code, response_buf);
                    } else {
                        ESP_LOGE("main", "Failed to read response");
                    }
                } else {
                    ESP_LOGE("main", "HTTP POST request failed: %s", esp_err_to_name(err));
                }
                esp_http_client_cleanup(client);
            }


            // clean up
            free(json_string);
        }

        // pause for a second
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
