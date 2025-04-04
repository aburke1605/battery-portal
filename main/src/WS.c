#include <esp_wifi.h>
#include <esp_http_client.h>
#include <esp_eap_client.h>

#include "include/WS.h"
#include "include/I2C.h"
#include "include/utils.h"

#include "include/cert.h"
#include "include/local_cert.h"

void add_client(int fd) {
    for (int i = 0; i < WS_CONFIG_MAX_CLIENTS; i++) {
        if (client_sockets[i] == -1) {
            client_sockets[i] = fd;
            ESP_LOGI("WS", "Client %d added", fd);
            return;
        }
    }
    ESP_LOGE("WS", "No space for client %d", fd);
}

void remove_client(int fd) {
    for (int i = 0; i < WS_CONFIG_MAX_CLIENTS; i++) {
        if (client_sockets[i] == fd) {
            client_sockets[i] = -1;
            ESP_LOGI("WS", "Client %d removed", fd);
            return;
        }
    }
}

esp_err_t perform_request(cJSON *message, cJSON *response) {
    // construct response message
    cJSON_AddStringToObject(response, "type", "response");
    cJSON_AddStringToObject(response, "esp_id", ESP_ID);
    cJSON *response_content = cJSON_CreateObject();

    cJSON *type = cJSON_GetObjectItem(message, "type");
    cJSON *content = cJSON_GetObjectItem(message, "content");
    if (!type || !content) {
        ESP_LOGE("WS", "Error in request message");
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
                ESP_LOGE("WS", "Failed to parse JSON");
                cJSON_AddStringToObject(response_content, "status", "error");
                return ESP_FAIL;
            }

            gpio_set_level(I2C_LED_GPIO_PIN, 1);

            const char* esp_id = cJSON_GetObjectItem(data, "new_esp_id")->valuestring;
            int BL = cJSON_GetObjectItem(data, "BL")->valueint;
            int BH = cJSON_GetObjectItem(data, "BH")->valueint;
            int CITL = cJSON_GetObjectItem(data, "CITL")->valueint;
            int CITH = cJSON_GetObjectItem(data, "CITH")->valueint;
            int CCT = cJSON_GetObjectItem(data, "CCT")->valueint;
            int DCT = cJSON_GetObjectItem(data, "DCT")->valueint;
            // use validate_change_handler logic directly
            // TODO: change the `websocket_event_handler` to do the same
            //       rather than using the `/validate_change` endpoint
            //       and sending mock htto requests to it

            if (esp_id != ESP_ID) {
                ESP_LOGI("I2C", "Changing device name...");
                set_device_name(I2C_DATA_SUBCLASS_ID, I2C_NAME_OFFSET, esp_id);
            }
            if (BL != test_read(I2C_DISCHARGE_SUBCLASS_ID, I2C_BL_OFFSET)) {
                ESP_LOGI("I2C", "Changing BL voltage...");
                set_I2_value(I2C_DISCHARGE_SUBCLASS_ID, I2C_BL_OFFSET, BL);
            }
            if (BH != test_read(I2C_DISCHARGE_SUBCLASS_ID, I2C_BH_OFFSET)) {
                ESP_LOGI("I2C", "Changing BH voltage...");
                set_I2_value(I2C_DISCHARGE_SUBCLASS_ID, I2C_BH_OFFSET, BH);
            }
            if (CITL != test_read(I2C_CHARGE_INHIBIT_CFG_SUBCLASS_ID, I2C_CHG_INHIBIT_TEMP_LOW_OFFSET)) {
                ESP_LOGI("I2C", "Changing charge inhibit low temperature threshold...");
                set_I2_value(I2C_CHARGE_INHIBIT_CFG_SUBCLASS_ID, I2C_CHG_INHIBIT_TEMP_LOW_OFFSET, CITL);
            }
            if (CITH != test_read(I2C_CHARGE_INHIBIT_CFG_SUBCLASS_ID, I2C_CHG_INHIBIT_TEMP_HIGH_OFFSET)) {
                ESP_LOGI("I2C", "Changing charge inhibit high temperature threshold...");
                set_I2_value(I2C_CHARGE_INHIBIT_CFG_SUBCLASS_ID, I2C_CHG_INHIBIT_TEMP_HIGH_OFFSET, CITH);
            }
            if (CCT != test_read(I2C_CHARGE_INHIBIT_CFG_SUBCLASS_ID, I2C_CHG_INHIBIT_TEMP_LOW_OFFSET)) {
                ESP_LOGI("I2C", "Changing charge current threshold...");
                set_I2_value(I2C_CURRENT_THRESHOLDS_SUBCLASS_ID, I2C_CHG_CURRENT_THRESHOLD_OFFSET, CCT);
            }
            if (DCT != test_read(I2C_CHARGE_INHIBIT_CFG_SUBCLASS_ID, I2C_CHG_INHIBIT_TEMP_HIGH_OFFSET)) {
                ESP_LOGI("I2C", "Changing discharge current threshold...");
                set_I2_value(I2C_CURRENT_THRESHOLDS_SUBCLASS_ID, I2C_DSG_CURRENT_THRESHOLD_OFFSET, DCT);
            }

            gpio_set_level(I2C_LED_GPIO_PIN, 0);

            cJSON_AddStringToObject(response_content, "status", "success");
        } else if (summary && strcmp(summary->valuestring, "connect-wifi") == 0) {
            cJSON *data = cJSON_GetObjectItem(content, "data");
            if (!data) {
                ESP_LOGE("WS", "Failed to parse JSON");
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
                    ESP_LOGI("AP", "Connecting to AP... SSID: eduroam");
                } else {
                    strncpy((char *)wifi_sta_config->sta.ssid, username, sizeof(wifi_sta_config->sta.ssid) - 1);
                    strncpy((char *)wifi_sta_config->sta.password, password, sizeof(wifi_sta_config->sta.password) - 1);
                    ESP_LOGI("AP", "Connecting to AP... SSID: %s", wifi_sta_config->sta.ssid);
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
                                ESP_LOGI("WS", "Connected to router. Signal strength: %d dBm", ap_info.rssi);
                                cJSON_AddStringToObject(response_content, "status", "success");

                                break;
                            }
                        }
                    } else {
                        if (VERBOSE) ESP_LOGI("WS", "Not connected. Retrying... %d", tries);
                        esp_wifi_connect();
                    }
                    tries++;

                    vTaskDelay(pdMS_TO_TICKS(1000));
                }
            }
        } else if (summary && strcmp(summary->valuestring, "reset-bms") == 0) {
            reset_BMS();
            cJSON_AddStringToObject(response_content, "status", "success");
        }
    } else {
        return ESP_ERR_NOT_SUPPORTED;
    }
    cJSON_AddItemToObject(response, "content", response_content);
    return ESP_OK;
}

esp_err_t validate_login_handler(httpd_req_t *req) {
    char content[100];
    esp_err_t err = get_POST_data(req, content, sizeof(content));
    if (err != ESP_OK) {
        ESP_LOGE("WS", "Problem with login POST request");
        return err;
    }

    char username_encoded[50] = {0};
    char password_encoded[50] = {0};
    // a correct login gets POSTed as:
    //    username=admin&password=1234
    sscanf(content, "username=%49[^&]&password=%49s", username_encoded, password_encoded);
    // %49:   read up to 49 characters (including the null terminator) to prevent buffer overflow
    // [^&]:  a scan set that matches any character except &
    // s:     reads a sequence of non-whitespace characters until a space, newline, or null terminator is encountered
    char username[50] = {0};
    char password[50] = {0};
    url_decode(username, username_encoded);
    url_decode(password, password_encoded);

    if (DEV || (strcmp(username, WS_USERNAME) == 0 && strcmp(password, WS_PASSWORD) == 0)) {
        // credentials correct
        admin_verified = true;
        httpd_resp_set_status(req, "302 Found");
        char redirect_url[25];
        snprintf(redirect_url, sizeof(redirect_url), "/esp32?esp_id=%s", ESP_ID);
        httpd_resp_set_hdr(req, "Location", redirect_url);
        httpd_resp_send(req, NULL, 0); // no response body
    } else {
        // credentials incorrect
        const char *error_msg = "Invalid username or password.";
        httpd_resp_send(req, error_msg, strlen(error_msg));
    }
    return ESP_OK;
}

esp_err_t websocket_handler(httpd_req_t *req) {
    // register new clients...
    if (req->method == HTTP_GET) {
        int fd = httpd_req_to_sockfd(req);
        ESP_LOGI("WS", "WebSocket handshake complete for client %d", fd);
        add_client(fd);
        return ESP_OK;  // WebSocket handshake happens here
    }

    // ...otherwise listen for messages
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    // first call with ws_pkt.len = 0 to determine required length
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGE("WS", "Failed to fetch WebSocket frame length: %s", esp_err_to_name(ret));
        return ret;
    }
    if (ws_pkt.len > 0) {
        // allocate buffer
        ws_pkt.payload = malloc(ws_pkt.len + 1);
        if (!ws_pkt.payload) {
            ESP_LOGE("WS", "Failed to allocate memory for WebSocket payload");
            return ESP_ERR_NO_MEM;
        }
        // receive payload
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK) {
            ESP_LOGE("WS", "Failed to receive WebSocket frame: %s", esp_err_to_name(ret));
            free(ws_pkt.payload);
            return ret;
        }
        ws_pkt.payload[ws_pkt.len] = '\0';
    }

    if (ws_pkt.type == HTTPD_WS_TYPE_TEXT) {
        if (VERBOSE) ESP_LOGI("WS", "Received WebSocket message: %s", (char *)ws_pkt.payload);

        // read messaage data
        cJSON *message = cJSON_Parse((char *)ws_pkt.payload);
        if (!message) {
            ESP_LOGE("WS", "Failed to parse JSON");
            free(ws_pkt.payload);
            return ESP_FAIL;
        }

        cJSON *response = cJSON_CreateObject();
        perform_request(message, response);

        cJSON_Delete(message);
    } else {
        ESP_LOGW("WS", "Received unsupported WebSocket frame type: %d", ws_pkt.type);
    }

    free(ws_pkt.payload);

    return ESP_OK;
}

esp_err_t file_serve_handler(httpd_req_t *req) {
    const char *file_path = (const char *)req->user_ctx;

    // before anything, make sure nobody tries to bypass the login page
    if (strcmp(file_path, "/static/esp32.html") == 0 && !admin_verified) {
        httpd_resp_set_status(req, "302 Found");
        httpd_resp_set_hdr(req, "Location", "/");
        httpd_resp_send(req, NULL, 0); // no response body
    }

    if (VERBOSE) ESP_LOGI("WS", "Serving file: %s", file_path);

    const char *ext = strrchr(file_path, '.');
    bool is_html = (ext && strcmp(ext, ".html") == 0);
    // only render html pages once to save memory
    if (is_html) {
        for (int i=0; i<n_rendered_html_pages; i++) {
            if (strcmp(file_path, rendered_html_pages[i].name) == 0) {
                httpd_resp_set_type(req, "text/html");
                httpd_resp_send(req, rendered_html_pages[i].content, HTTPD_RESP_USE_STRLEN);

                return ESP_OK;
            }
        }

        char *content_html = read_file(file_path);
        if (!content_html) {
            ESP_LOGE("WS", "Failed to serve HTML file");
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found");
            return ESP_FAIL;
        }

        struct rendered_page* new_page = malloc(sizeof(struct rendered_page));

        strncpy(new_page->name, file_path, WS_MAX_HTML_PAGE_NAME_LENGTH - 1);
        new_page->name[WS_MAX_HTML_PAGE_NAME_LENGTH - 1] = '\0';

        strncpy(new_page->content, content_html, WS_MAX_HTML_SIZE - 1);
        new_page->content[WS_MAX_HTML_SIZE - 1] = '\0';

        rendered_html_pages[n_rendered_html_pages++] = *new_page;
        free(new_page);

        httpd_resp_set_type(req, "text/html");
        httpd_resp_send(req, content_html, HTTPD_RESP_USE_STRLEN);

        return ESP_OK;
    }


    // for non-HTML files, serve normally
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
    if (ext) {
        if (strcmp(ext, ".css") == 0) {
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
    for (int i = 0; i < WS_CONFIG_MAX_CLIENTS; i++) {
        client_sockets[i] = -1;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.max_uri_handlers = 32; // Increase this number as needed
    config.max_open_sockets = CONFIG_LWIP_MAX_SOCKETS - 3;

    // Start the httpd server
    ESP_LOGI("WS", "Starting server on port: '%d'", config.server_port);

    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI("WS", "Registering URI handlers");

        httpd_uri_t login_uri = {
            .uri       = "/",
            .method    = HTTP_GET,
            .handler   = file_serve_handler,
            .user_ctx  = "/static/esp_login.html"
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

        httpd_uri_t validate_login_uri = {
            .uri       = "/validate_login",
            .method    = HTTP_POST,
            .handler   = validate_login_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &validate_login_uri);

        httpd_uri_t ws_uri = {
            .uri = "/browser_ws",
            .method = HTTP_GET,
            .handler = websocket_handler,
            .is_websocket = true
        };
        httpd_register_uri_handler(server, &ws_uri);

        httpd_uri_t esp32_uri = {
            .uri       = "/esp32",
            .method    = HTTP_GET,
            .handler   = file_serve_handler,
            .user_ctx  = "/static/esp32.html"
        };
        httpd_register_uri_handler(server, &esp32_uri);

        httpd_uri_t favicon_uri = {
            .uri       = "/favicon.png",
            .method    = HTTP_GET,
            .handler   = file_serve_handler,
            .user_ctx  = "/static/favicon.png"
        };
        httpd_register_uri_handler(server, &favicon_uri);

        httpd_uri_t js_uri = {
            .uri      = "/assets/esp.js",
            .method   = HTTP_GET,
            .handler  = file_serve_handler,
            .user_ctx = "/static/assets/esp.js",
        };
        httpd_register_uri_handler(server, &js_uri);

        js_uri.uri = "/assets/zap.js";
        js_uri.user_ctx = "/static/assets/zap.js";
        httpd_register_uri_handler(server, &js_uri);

        js_uri.uri = "/assets/mock-socket.js";
        js_uri.user_ctx = "/static/assets/mock-socket.js";
        httpd_register_uri_handler(server, &js_uri);

        httpd_uri_t css_uri = {
            .uri      = "/assets/zap.css",
            .method   = HTTP_GET,
            .handler  = file_serve_handler,
            .user_ctx = "/static/assets/zap.css",
        };
        httpd_register_uri_handler(server, &css_uri);

        css_uri.uri = "/assets/mock-socket.css";
        css_uri.user_ctx = "/static/assets/mock-socket.css";
        httpd_register_uri_handler(server, &css_uri);

    } else {
        ESP_LOGE("WS", "Error starting server!");
    }
    return server;
}

void send_ws_message(const char *message) {
    if (xQueueSend(ws_queue, message, portMAX_DELAY) != pdPASS) {
        ESP_LOGE("WS", "WebSocket queue full! Dropping message: %s", message);
    }
}

void websocket_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    esp_websocket_event_data_t *ws_event_data = (esp_websocket_event_data_t *)event_data;

    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            if (VERBOSE) ESP_LOGI("WS", "WebSocket connected");
            char websocket_connect_message[128];
            snprintf(websocket_connect_message, sizeof(websocket_connect_message), "{\"type\": \"register\", \"esp_id\": \"%s\"}", ESP_ID);
            send_ws_message(websocket_connect_message);
            break;

        case WEBSOCKET_EVENT_DISCONNECTED:
            if (VERBOSE) ESP_LOGI("WS", "WebSocket disconnected");
            esp_websocket_client_stop(ws_client);
            break;

        case WEBSOCKET_EVENT_DATA:
            if (ws_event_data->data_len == 0) break;

            cJSON *message = cJSON_Parse((char *)ws_event_data->data_ptr);
            if (!message) {
                ESP_LOGE("WS", "invalid json");
                cJSON_Delete(message);
                break;
            }

            cJSON *response = cJSON_CreateObject();
            esp_err_t err = perform_request(message, response);

            char *response_str = cJSON_PrintUnformatted(response);
            cJSON_Delete(response);
            if (err == ESP_OK) send_ws_message(response_str);
            free(response_str);
            cJSON_Delete(message);

            break;

        case WEBSOCKET_EVENT_ERROR:
            if (VERBOSE) ESP_LOGE("WS", "WebSocket error occurred");
            break;

        default:
            if (VERBOSE) ESP_LOGI("WS", "WebSocket event ID: %ld", event_id);
            break;
    }
}

void websocket_task(void *pvParameters) {
    char message[WS_MESSAGE_MAX_LEN];
    while (true) {
        if (DEV) send_fake_request();

        // read sensor data
        uint16_t iCharge = read_2byte_data(I2C_STATE_OF_CHARGE_REG);
        uint16_t iVoltage = read_2byte_data(I2C_VOLTAGE_REG);
        float fVoltage = (float)iVoltage / 1000.0;
        uint16_t iCurrent = read_2byte_data(I2C_CURRENT_REG);
        float fCurrent = (float)iCurrent / 1000.0;
        if (fCurrent < 65.536 && fCurrent > 32.767) fCurrent = 65.536 - fCurrent; // this is something to do with 16 bit binary
        uint16_t iTemperature = read_2byte_data(I2C_TEMPERATURE_REG);
        float fTemperature = (float)iTemperature / 10.0 - 273.15;

        // configurable data too
        read_name(I2C_DATA_SUBCLASS_ID, I2C_NAME_OFFSET, ESP_ID);
        uint16_t iBL = test_read(I2C_DISCHARGE_SUBCLASS_ID, I2C_BL_OFFSET);
        uint16_t iBH = test_read(I2C_DISCHARGE_SUBCLASS_ID, I2C_BH_OFFSET);
        uint16_t iCCT = test_read(I2C_CURRENT_THRESHOLDS_SUBCLASS_ID, I2C_CHG_CURRENT_THRESHOLD_OFFSET);
        uint16_t iDCT = test_read(I2C_CURRENT_THRESHOLDS_SUBCLASS_ID, I2C_DSG_CURRENT_THRESHOLD_OFFSET);
        uint16_t iCITL = test_read(I2C_CHARGE_INHIBIT_CFG_SUBCLASS_ID, I2C_CHG_INHIBIT_TEMP_LOW_OFFSET);
        uint16_t iCITH = test_read(I2C_CHARGE_INHIBIT_CFG_SUBCLASS_ID, I2C_CHG_INHIBIT_TEMP_HIGH_OFFSET);

        // create JSON object with sensor data
        cJSON *json = cJSON_CreateObject();
        cJSON *data = cJSON_CreateObject();

        cJSON_AddNumberToObject(data, "charge", iCharge);
        cJSON_AddNumberToObject(data, "voltage", fVoltage);
        cJSON_AddNumberToObject(data, "current", fCurrent);
        cJSON_AddNumberToObject(data, "temperature", fTemperature);
        cJSON_AddNumberToObject(data, "BL", iBL);
        cJSON_AddNumberToObject(data, "BH", iBH);
        cJSON_AddNumberToObject(data, "CCT", iCCT);
        cJSON_AddNumberToObject(data, "DCT", iDCT);
        cJSON_AddNumberToObject(data, "CITL", iCITL);
        cJSON_AddNumberToObject(data, "CITH", iCITH);
        cJSON_AddStringToObject(data, "IP", ESP_IP);
        cJSON_AddBoolToObject(data, "connected_to_WiFi", connected_to_WiFi);
        char *data_string = cJSON_PrintUnformatted(data);

        cJSON_AddItemToObject(json, ESP_ID, data);
        char *json_string = cJSON_PrintUnformatted(json);

        // cJSON_Delete(data);
        cJSON_Delete(json);

        if (json_string != NULL && data_string != NULL) {
            // first send to all connected WebSocket clients
            for (int i = 0; i < WS_CONFIG_MAX_CLIENTS; i++) {
                if (client_sockets[i] != -1) {
                    httpd_ws_frame_t ws_pkt = {
                        .payload = (uint8_t *)json_string,
                        .len = strlen(json_string),
                        .type = HTTPD_WS_TYPE_TEXT,
                    };

                    esp_err_t err = httpd_ws_send_frame_async(server, client_sockets[i], &ws_pkt);
                    if (err != ESP_OK) {
                        ESP_LOGE("WS", "Failed to send frame to client %d: %s", client_sockets[i], esp_err_to_name(err));
                        remove_client(client_sockets[i]);  // Clean up disconnected clients
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
                        ESP_LOGE("WS", "Failed to initialize WebSocket client");
                        vTaskDelay(pdMS_TO_TICKS(1000));
                        continue;
                    }
                    esp_websocket_register_events(ws_client, WEBSOCKET_EVENT_ANY, websocket_event_handler, NULL);
                    if (esp_websocket_client_start(ws_client) != ESP_OK) {
                        ESP_LOGE("WS", "Failed to start WebSocket client");
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
                    snprintf(message, sizeof(message), "{\"type\": \"data\", \"esp_id\": \"%s\", \"content\": %s}", ESP_ID, data_string);
                    send_ws_message(message);
                }
            }

            // clean up
            free(data_string);
            free(json_string);
        }

        // check wifi connection still exists
        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
            connected_to_WiFi = false;
        }

        // send the next message in queue
        if (xQueueReceive(ws_queue, message, portMAX_DELAY) == pdPASS) {
            if (esp_websocket_client_is_connected(ws_client)) {
                if (VERBOSE) ESP_LOGI("WS", "Sending: %s", message);
                esp_websocket_client_send_text(ws_client, message, strlen(message), portMAX_DELAY);
            } else {
                ESP_LOGW("WS", "WebSocket not connected, dropping message: %s", message);
            }
        }

        check_bytes((TaskParams *)pvParameters);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
