#include <esp_wifi.h>
#include <esp_http_client.h>

#include "include/WS.h"
#include "include/I2C.h"
#include "include/utils.h"

esp_websocket_client_handle_t ws_client;

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
    if (err != ESP_OK) {
        ESP_LOGE("WS", "Problem with login POST request");
        return err;
    }

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
    if (err != ESP_OK) {
        ESP_LOGE("WS", "Problem with change POST request");
        return err;
    }

    // Check if each parameter exists and parse it
    char *BL_start = strstr(content, "BL_voltage_threshold=");
    char *BH_start = strstr(content, "BH_voltage_threshold=");
    char *CCT_start = strstr(content, "charge_current_threshold=");
    char *DCT_start = strstr(content, "discharge_current_threshold=");
    char *CITL_start = strstr(content, "chg_inhibit_temp_low=");
    char *CITH_start = strstr(content, "chg_inhibit_temp_high=");

    static bool led_on = false;
    led_on = !led_on;
    gpio_set_level(LED_GPIO_PIN, led_on ? 1 : 0);

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

    vTaskDelay(pdMS_TO_TICKS(500));
    gpio_set_level(LED_GPIO_PIN, led_on ? 0 : 1);

    return ESP_OK;
}

esp_err_t reset_handler(httpd_req_t *req) {
    reset_BMS();

    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/change"); // redirect to /change
    httpd_resp_send(req, NULL, 0); // no response body

    return ESP_OK;
}

esp_err_t validate_connect_handler(httpd_req_t *req) {
    char content[100];
    esp_err_t err;

    if (req->user_ctx != NULL) {
        // request came from a WebSocket
        strncpy(content, (const char *)req->user_ctx, sizeof(content) - 1);
        content[sizeof(content) - 1] = '\0';
    } else{
        // request is a real HTTP POST
        err = get_POST_data(req, content, sizeof(content));
        if (err != ESP_OK) {
            ESP_LOGE("WS", "Problem with connect POST request");
            return err;
        }
    }

    char ssid_encoded[50] = {0};
    char ssid[50] = {0};
    char password_encoded[50] = {0};
    char password[50] = {0};
    sscanf(content, "ssid=%49[^&]&password=%49s", ssid_encoded, password_encoded);
    url_decode(ssid, ssid_encoded);
    url_decode(password, password_encoded);

    int tries = 0;
    int max_tries = 10;

    if (!connected_to_WiFi) {
        wifi_config_t *wifi_sta_config = malloc(sizeof(wifi_config_t));
        memset(wifi_sta_config, 0, sizeof(wifi_config_t));

        strncpy((char *)wifi_sta_config->sta.ssid, ssid, sizeof(wifi_sta_config->sta.ssid) - 1);
        strncpy((char *)wifi_sta_config->sta.password, password, sizeof(wifi_sta_config->sta.password) - 1);

        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_sta_config));

        ESP_LOGI("AP", "Connecting to AP... SSID: %s", wifi_sta_config->sta.ssid);

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

                        ESP_LOGI("WS", "Connected to router. Signal strength: %d dBm", ap_info.rssi);
                        if (req->handle) {
                            httpd_resp_set_status(req, "302 Found");
                            httpd_resp_set_hdr(req, "Location", "/display"); // redirect back to /display
                            httpd_resp_send(req, NULL, 0); // no response body
                        } else {
                            req->user_ctx = "success";
                        }

                        break;
                    }
                }
            } else {
                ESP_LOGI("WS", "Not connected. Retrying... %d", tries);
                esp_wifi_connect();
            }
            tries++;

            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    } else {
        ESP_LOGW("WS", "Already connected to Wi-Fi. Redirecting...");
        if (req->handle) {
            char message[] = "Already connected to Wi-Fi";
            char encoded_message[64];
            url_encode(encoded_message, message, sizeof(encoded_message));

            char redirect_url[128];
            snprintf(redirect_url, sizeof(redirect_url), "/alert?message=%s", encoded_message);
            httpd_resp_set_status(req, "302 Found");
            httpd_resp_set_hdr(req, "Location", redirect_url);
            httpd_resp_send(req, NULL, 0);
        } else {
            req->user_ctx = "already connected";
        }
    }

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

esp_err_t alert_handler(httpd_req_t *req) {
    char message[100] = "Missing message";

    // extract message from query params if available
    char query[200];
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        httpd_query_key_value(query, "message", message, sizeof(message));
    }

    // Open the file
    char *html_buffer = (char *)malloc(512);  // adjust based on file size
    if (!html_buffer) {
        return httpd_resp_send_500(req);
    }
    FILE *file = fopen("/templates/alert.html", "r");
    if (!file) {
        ESP_LOGE("WS", "Failed to open file: /templates/alert.html");
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found");
        free(html_buffer);
        return ESP_FAIL;
    }
    fread(html_buffer, 1, 511, file);
    html_buffer[511] = '\0';
    fclose(file);

    // Find and replace {{message}}
    char *placeholder = strstr(html_buffer, "{{message}}");
    if (placeholder) {
        char *final_html = (char *)malloc(512);// Adjust based on expected output size
        if (!final_html) {
            free(html_buffer);
            return httpd_resp_send_500(req);
        }
        size_t before_len = placeholder - html_buffer;
        snprintf(final_html, 512, "%.*s%s%s",
                 (int)before_len, html_buffer, message, placeholder + 11); // Skip `{{message}}`

        // Send the modified HTML response
        httpd_resp_set_type(req, "text/html");
        httpd_resp_send(req, final_html, HTTPD_RESP_USE_STRLEN);

        free(html_buffer);
        free(final_html);

        return ESP_OK;
    }

    // If no placeholder found, send the original file
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html_buffer, HTTPD_RESP_USE_STRLEN);

    free(html_buffer);

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
            .user_ctx  = "/templates/login.html"
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
        login_uri.uri = "/wpad.dat";
        httpd_register_uri_handler(server, &login_uri);
        login_uri.uri = "/gen_204";
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
            .user_ctx  = "/templates/display.html"
        };
        httpd_register_uri_handler(server, &display_uri);

        httpd_uri_t alert_uri = {
            .uri       = "/alert",
            .method    = HTTP_GET,
            .handler   = alert_handler,
            .user_ctx  = "/templates/alert.html"
        };
        httpd_register_uri_handler(server, &alert_uri);

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
            .user_ctx  = "/templates/change.html"
        };
        httpd_register_uri_handler(server, &change_uri);

        httpd_uri_t validate_change_uri = {
            .uri       = "/validate_change",
            .method    = HTTP_POST,
            .handler   = validate_change_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &validate_change_uri);

        httpd_uri_t reset_uri = {
            .uri       = "/reset",
            .method    = HTTP_POST,
            .handler   = reset_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &reset_uri);

        // Connect page
        httpd_uri_t connect_uri = {
            .uri       = "/connect",
            .method    = HTTP_GET,
            .handler   = file_serve_handler,
            .user_ctx  = "/templates/connect.html"
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
            .user_ctx  = "/templates/nearby.html"
        };
        httpd_register_uri_handler(server, &nearby_uri);

        // About page
        httpd_uri_t about_uri = {
            .uri       = "/about",
            .method    = HTTP_GET,
            .handler   = file_serve_handler,
            .user_ctx  = "/templates/about.html"
        };
        httpd_register_uri_handler(server, &about_uri);

        // Device page
        httpd_uri_t device_uri = {
            .uri       = "/device",
            .method    = HTTP_GET,
            .handler   = file_serve_handler,
            .user_ctx  = "/templates/device.html"
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
            .uri      = "/static/portal/style.css",
            .method   = HTTP_GET,
            .handler  = css_handler,
            .user_ctx = "/static/style.css",
        };
        httpd_register_uri_handler(server, &css_uri);

        // Add a handler for serving the image
        httpd_uri_t image_uri = {
            .uri       = "/static/portal/images/aceon.png",
            .method    = HTTP_GET,
            .handler   = image_handler, // Function to read and send the image
            .user_ctx  = "/static/images/aceon.png" // File path as user context
        };
        httpd_register_uri_handler(server, &image_uri);

        httpd_uri_t image_uri_2 = {
            .uri       = "/static/portal/images/aceon2.png",
            .method    = HTTP_GET,
            .handler   = image_handler, // Function to read and send the image
            .user_ctx  = "/static/images/aceon2.png" // File path as user context
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

        cJSON_AddStringToObject(json, "IP", ESP_IP);

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
                for (size_t i = 0; i < old_successful_ip_count; i++) {
                    char url[33];
                    snprintf(url, sizeof(url), "http://%s:5000/data", old_successful_ips[i]);
                    esp_http_client_config_t config = {
                        .url = url,
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
                            ESP_LOGI("WS", "HTTP POST Status = %d, Response = %s", status_code, response_buf);
                        } else {
                            ESP_LOGE("WS", "Failed to read response");
                        }
                    } else {
                        ESP_LOGE("WS", "HTTP POST request failed: %s", esp_err_to_name(err));
                    }
                    esp_http_client_cleanup(client);
                }
            }


            // clean up
            free(json_string);
        }

        // pause for a second
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void check_wifi_task(void* pvParameters) {
    while(true) {
        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
            connected_to_WiFi = false;
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void websocket_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    esp_websocket_event_data_t *ws_event_data = (esp_websocket_event_data_t *)event_data;

    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            ESP_LOGI("WS", "WebSocket connected");
            char websocket_connect_message[128];
            snprintf(websocket_connect_message, sizeof(websocket_connect_message), "{'ESP_ID': '%s'}", ESP_ID);
            esp_websocket_client_send_text(ws_client, websocket_connect_message, strlen(websocket_connect_message), portMAX_DELAY);
            connected_to_website = true;
            break;

        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGI("WS", "WebSocket disconnected");
            esp_websocket_client_stop(ws_client);
            connected_to_website = false;
            break;

        case WEBSOCKET_EVENT_DATA:
            if (ws_event_data->data_len == 0) break;

            cJSON *message = cJSON_Parse((char *)ws_event_data->data_ptr);
            if (!message) {
                ESP_LOGE("WS", "invalid json");
                cJSON_Delete(message);
                break;
            }

            cJSON *typeItem = cJSON_GetObjectItem(message, "type");
            if (!typeItem) {
                ESP_LOGE("WS", "ilformatted json");
                break;
            }
            const char *type = typeItem->valuestring;

            ESP_LOGI("WS", "WebSocket data received: %.*s", ws_event_data->data_len, (char *)ws_event_data->data_ptr);

            if (strcmp(type, "response") == 0) {

            } else if (strcmp(type, "request") == 0) {
                cJSON *content = cJSON_GetObjectItem(message, "content");
                if (!content) {
                    ESP_LOGE("WS", "invalid request content");
                    cJSON_Delete(message);
                    break;
                }

                const char *endpoint = cJSON_GetObjectItem(content, "endpoint")->valuestring;
                const char *method = cJSON_GetObjectItem(content, "method")->valuestring;
                cJSON *data = cJSON_GetObjectItem(content, "data");
                if (strcmp(endpoint, "validate_connect") == 0 && strcmp(method, "POST") == 0) {
                    // Create a mock HTTP request
                    cJSON *ssid = cJSON_GetObjectItem(data, "ssid");
                    cJSON *password = cJSON_GetObjectItem(data, "password");

                    size_t req_len = snprintf(NULL, 0, "ssid=%s&password=%s", ssid->valuestring, password->valuestring);
                    char *req_content = malloc(req_len + 1);
                    snprintf(req_content, req_len + 1, "ssid=%s&password=%s", ssid->valuestring, password->valuestring);

                    httpd_req_t req = {0};
                    req.content_len = req_len;
                    req.user_ctx = req_content;

                    // Call the validate_connect_handler
                    esp_err_t err = validate_connect_handler(&req);
                    if (err != ESP_OK) {
                        ESP_LOGE("WS", "Error in validate_connect_handler: %d", err);
                    }
                    free(req_content);

                    cJSON *response_content = cJSON_CreateObject();
                    cJSON_AddStringToObject(response_content, "endpoint", endpoint);
                    cJSON_AddStringToObject(response_content, "status", err == ESP_OK ? "success" : "error");
                    cJSON_AddNumberToObject(response_content, "error_code", err);
                    cJSON_AddStringToObject(response_content, "response", req.user_ctx);

                    cJSON *response = cJSON_CreateObject();
                    cJSON_AddStringToObject(response, "type", "response");
                    cJSON_AddItemToObject(response, "content", response_content);

                    char *response_str = cJSON_PrintUnformatted(response);
                    cJSON_Delete(response);
                    esp_err_t send_err = esp_websocket_client_send_text(ws_client, response_str, strlen(response_str), portMAX_DELAY);
                    if (send_err != ESP_OK) {
                        ESP_LOGE("WS", "Failed to send WebSocket response: %d", send_err);
                    }
                    free(response_str);
                }
            }

            cJSON_Delete(message);
            break;

        case WEBSOCKET_EVENT_ERROR:
            ESP_LOGE("WS", "WebSocket error occurred");
            break;

        default:
            ESP_LOGI("WS", "WebSocket event ID: %ld", event_id);
            break;
    }
}

void websocket_reconnect_task(void *param) {
    while (true) {
        if (connected_to_WiFi) {
            if (!connected_to_website) {
                const esp_websocket_client_config_t websocket_cfg = {
                    .uri = "ws://192.168.137.249:5000/ws",
                    .reconnect_timeout_ms = 1000,
                    .network_timeout_ms = 10000,
                };

                ESP_LOGI("WS", "Connecting to WebSocket server: %s", websocket_cfg.uri);

                ws_client = esp_websocket_client_init(&websocket_cfg);
                esp_websocket_register_events(ws_client, WEBSOCKET_EVENT_ANY, websocket_event_handler, NULL);
                esp_websocket_client_start(ws_client);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5000)); // Retry every 5 seconds
    }
}
