#include <esp_wifi.h>
#include <esp_http_client.h>
#include <esp_eap_client.h>

#include "include/WS.h"
#include "include/I2C.h"
#include "include/utils.h"

#include "include/cert.h"
#include "include/local_cert.h"

static struct rendered_page rendered_html_pages[WS_MAX_N_HTML_PAGES];
static int client_sockets[WS_CONFIG_MAX_CLIENTS];
static bool reconnect = false;
static char ESP_IP[16] = "xxx.xxx.xxx.xxx\0";
static esp_websocket_client_handle_t ws_client = NULL;
static uint8_t n_rendered_html_pages = 0;

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

    if (strcmp(username, WS_USERNAME) == 0 && strcmp(password, WS_PASSWORD) == 0) {
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

    // Check if each parameter exists and parse it
    char *name_start = strstr(content, "device_name=");
    char *BL_start = strstr(content, "BL_voltage_threshold=");
    char *BH_start = strstr(content, "BH_voltage_threshold=");
    char *CCT_start = strstr(content, "charge_current_threshold=");
    char *DCT_start = strstr(content, "discharge_current_threshold=");
    char *CITL_start = strstr(content, "chg_inhibit_temp_low=");
    char *CITH_start = strstr(content, "chg_inhibit_temp_high=");

    static bool led_on = false;
    led_on = !led_on;
    gpio_set_level(I2C_LED_GPIO_PIN, led_on ? 1 : 0);

    if (name_start) {
        char device_name[11] = {0};
        sscanf(name_start, "device_name=%10[^&]", device_name);
        device_name[sizeof(device_name)] = '\0';
        if (device_name[0] != '\0') {
            ESP_LOGI("I2C", "Changing device name...");
            set_device_name(I2C_DATA_SUBCLASS_ID, I2C_NAME_OFFSET, device_name);
        }
    }

    if (BL_start) {
        char BL_voltage_threshold[50] = {0};
        sscanf(BL_start, "BL_voltage_threshold=%49[^&]", BL_voltage_threshold);
        if (BL_voltage_threshold[0] != '\0') {
            ESP_LOGI("I2C", "Changing BL voltage...");
            set_I2_value(I2C_DISCHARGE_SUBCLASS_ID, I2C_BL_OFFSET, atoi(BL_voltage_threshold));
        }
    }

    if (BH_start) {
        char BH_voltage_threshold[50] = {0};
        sscanf(BH_start, "BH_voltage_threshold=%49[^&]", BH_voltage_threshold);
        if (BH_voltage_threshold[0] != '\0') {
            ESP_LOGI("I2C", "Changing BH voltage...");
            set_I2_value(I2C_DISCHARGE_SUBCLASS_ID, I2C_BH_OFFSET, atoi(BH_voltage_threshold));
        }
    }

    if (CCT_start) {
        char charge_current_threshold[50] = {0};
        sscanf(CCT_start, "charge_current_threshold=%49[^&]", charge_current_threshold);
        if (charge_current_threshold[0] != '\0') {
            ESP_LOGI("I2C", "Changing charge current threshold...");
            set_I2_value(I2C_CURRENT_THRESHOLDS_SUBCLASS_ID, I2C_CHG_CURRENT_THRESHOLD_OFFSET, atoi(charge_current_threshold));
        }
    }

    if (DCT_start) {
        char discharge_current_threshold[50] = {0};
        sscanf(DCT_start, "discharge_current_threshold=%49[^&]", discharge_current_threshold);
        if (discharge_current_threshold[0] != '\0') {
            ESP_LOGI("I2C", "Changing discharge current threshold...");
            set_I2_value(I2C_CURRENT_THRESHOLDS_SUBCLASS_ID, I2C_DSG_CURRENT_THRESHOLD_OFFSET, atoi(discharge_current_threshold));
        }
    }

    if (CITL_start) {
        char chg_inhibit_temp_low[50] = {0};
        sscanf(CITL_start, "chg_inhibit_temp_low=%49[^&]", chg_inhibit_temp_low);
        if (chg_inhibit_temp_low[0] != '\0') {
            ESP_LOGI("I2C", "Changing charge inhibit low temperature threshold...");
            set_I2_value(I2C_CHARGE_INHIBIT_CFG_SUBCLASS_ID, I2C_CHG_INHIBIT_TEMP_LOW_OFFSET, atoi(chg_inhibit_temp_low));
        }
    }

    if (CITH_start) {
        char chg_inhibit_temp_high[50] = {0};
        sscanf(CITH_start, "chg_inhibit_temp_high=%49s", chg_inhibit_temp_high);
        if (chg_inhibit_temp_high[0] != '\0') {
            ESP_LOGI("I2C", "Changing charge inhibit high temperature threshold...");
            set_I2_value(I2C_CHARGE_INHIBIT_CFG_SUBCLASS_ID, I2C_CHG_INHIBIT_TEMP_HIGH_OFFSET, atoi(chg_inhibit_temp_high));
        }
    }

    vTaskDelay(pdMS_TO_TICKS(500));
    gpio_set_level(I2C_LED_GPIO_PIN, led_on ? 0 : 1);

    req->user_ctx = "success";

    return ESP_OK;
}

esp_err_t reset_handler(httpd_req_t *req) {
    write_data(I2C_CONTROL_REG, I2C_CONTROL_RESET_SUBCMD, 2);

    // delay to allow reset to complete
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI("I2C", "Reset command sent successfully.");

    if (req->handle) {
        // request is a real HTTP POST
        httpd_resp_set_status(req, "302 Found");
        httpd_resp_set_hdr(req, "Location", "/change"); // redirect to /change
        httpd_resp_send(req, NULL, 0); // no response body
    } else {
        req->user_ctx = "success";
    }

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

    if (!connected_to_WiFi || reconnect) {
        if (reconnect) {
            connected_to_WiFi = false;
        }

        wifi_config_t *wifi_sta_config = malloc(sizeof(wifi_config_t));
        memset(wifi_sta_config, 0, sizeof(wifi_config_t));

        if (strcmp(req->uri, "/validate_connect?eduroam=true") == 0) {
            strncpy((char *)wifi_sta_config->sta.ssid, "eduroam", 8);

            esp_wifi_sta_enterprise_enable();
            esp_eap_client_set_username((uint8_t *)ssid, strlen(ssid));
            esp_eap_client_set_password((uint8_t *)password, strlen(password));
        } else {
            strncpy((char *)wifi_sta_config->sta.ssid, ssid, sizeof(wifi_sta_config->sta.ssid) - 1);
            strncpy((char *)wifi_sta_config->sta.password, password, sizeof(wifi_sta_config->sta.password) - 1);
        }

        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_sta_config));
        // TODO: if reconnecting, it doesn't actually seem to drop the old connection in favour of the new one

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
                if (VERBOSE) ESP_LOGI("WS", "Not connected. Retrying... %d", tries);
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
    gpio_set_level(I2C_LED_GPIO_PIN, led_on ? 1 : 0);
    ESP_LOGI("WS", "LED is now %s", led_on ? "ON" : "OFF");

    if (req->handle) {
        char message[] = "LED Toggled";
        char encoded_message[64];
        url_encode(encoded_message, message, sizeof(encoded_message));

        char redirect_url[128];
        snprintf(redirect_url, sizeof(redirect_url), "/alert?message=%s", encoded_message);
        httpd_resp_set_status(req, "302 Found");
        httpd_resp_set_hdr(req, "Location", redirect_url);
        httpd_resp_send(req, NULL, 0);
    } else {
        req->user_ctx = "led toggled";
    }
    return ESP_OK;
}

esp_err_t file_serve_handler(httpd_req_t *req) {
    const char *file_path = (const char *)req->user_ctx; // Get file path from user_ctx
    if (VERBOSE) ESP_LOGI("WS", "Serving file: %s", file_path);

    bool already_rendered = false;
    for (int i=0; i<n_rendered_html_pages; i++) {
        if (strcmp(file_path, rendered_html_pages[i].name) == 0) {
            already_rendered = true;

            httpd_resp_set_type(req, "text/html");
            httpd_resp_send(req, rendered_html_pages[i].content, HTTPD_RESP_USE_STRLEN);

            return ESP_OK;
        }
    }

    const char *ext = strrchr(file_path, '.');
    bool is_html = (ext && strcmp(ext, ".html") == 0);

    // for HTML files, remove jinja lines
    // and do merging with base.html by hand
    if (is_html) {
        char *base_html = read_file("/templates/base.html");
        char *content_html = read_file(file_path);
        if (!base_html || !content_html) {
            ESP_LOGE("WS", "Failed to read base or content HTML file");
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found");
            return ESP_FAIL;
        }

        if (strlen(base_html) + strlen(content_html) + 1 >= WS_MAX_HTML_SIZE) {
            ESP_LOGE("WS", "Response buffer overflow risk!");
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Buffer overflow");
            return ESP_FAIL;
        }


        char *page = malloc(WS_MAX_HTML_SIZE);
        if (!page) {
            ESP_LOGE("WS", "Failed to allocate memory for page");
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
            return ESP_FAIL;
        }


        const char* base_jinja_string;
        char* base_insert_pos;
        const char* content_jinja_string_1;
        const char* content_jinja_string_2;

        // remove jinja from base and add to page
        base_jinja_string = "    {% block content %}{% endblock %}";
        base_insert_pos = strstr(base_html, base_jinja_string);
        if (!base_insert_pos) {
            ESP_LOGE("WS", "Template placeholder not found in base.html");
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Template error");
            return ESP_FAIL;
        }

        // add start section of base to rendered page
        size_t base_start_len = base_insert_pos - base_html;
        strncpy(page, base_html, base_start_len);
        page[base_start_len-1] = '\0';

        // remove jinja from content first and add to page
        content_jinja_string_1 = "{% extends \"portal/base.html\" %}\n\n\n{% block content %}";
        content_jinja_string_2 = "{% endblock %}";
        strncat(page, content_html + strlen(content_jinja_string_1), strlen(content_html) - strlen(content_jinja_string_1) - strlen(content_jinja_string_2) - 1);

        // add end section of base to rendered page
        // strncat(page, base_insert_pos + strlen(base_jinja_string), sizeof(page) - strlen(page) - 1);
        strcat(page, "</body>\n</html>\n\n");

        // remove other jinja bits too
        char ESP_ID_string[sizeof(ESP_ID) + 2];
        snprintf(ESP_ID_string, sizeof(ESP_ID_string), "\"%s\"", ESP_ID);

        const char* placeholders[] = {
            "wss://",
            "{{ prefix }}",
            "\"{{ esp_id }}\"",
            "?esp_id={{ esp_id }}",
            "&" // fixes eduroam.html
        };
        const char* substitutes[] = {
            "ws://",
            "",
            ESP_ID_string,
            "",
            "?"
        };
        char *modified_page = replace_placeholder(page, placeholders, substitutes, 5);

        if (!modified_page) {
            ESP_LOGE("WS", "Could not replace {{ prefix }} in %s", file_path);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Template error");
            return ESP_FAIL;
        }

        if (!already_rendered) {
            struct rendered_page* new_page = malloc(sizeof(struct rendered_page));

            strncpy(new_page->name, file_path, WS_MAX_HTML_PAGE_NAME_LENGTH - 1);
            new_page->name[WS_MAX_HTML_PAGE_NAME_LENGTH - 1] = '\0';

            strncpy(new_page->content, modified_page, WS_MAX_HTML_SIZE - 1);
            new_page->content[WS_MAX_HTML_SIZE - 1] = '\0';

            rendered_html_pages[n_rendered_html_pages++] = *new_page;
            free(new_page);
        }

        httpd_resp_set_type(req, "text/html");
        httpd_resp_send(req, modified_page, HTTPD_RESP_USE_STRLEN);

        free(modified_page);
        free(page);

        free(base_html);
        free(content_html);

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

    // remove {{ prefix }} jinja bits too
    // (replace with nothing)
    const char* placeholders[] = {"{{ prefix }}"};
    const char* substitutes[] = {""};
    char *modified_page = replace_placeholder(html_buffer, placeholders, substitutes, 1);
    if (!modified_page) {
        ESP_LOGE("WS", "Could not replace {{ prefix }} in %s", "/templates/alert.html");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Template error");
        return ESP_FAIL;
    }

    // Find and replace {{message}}
    char *placeholder = strstr(modified_page, "{{message}}");
    if (placeholder) {
        char *final_html = (char *)malloc(512);// Adjust based on expected output size
        if (!final_html) {
            free(modified_page);
            free(html_buffer);
            return httpd_resp_send_500(req);
        }
        size_t before_len = placeholder - modified_page;
        snprintf(final_html, 512, "%.*s%s%s",
                 (int)before_len, modified_page, message, placeholder + 11); // Skip `{{message}}`

        // Send the modified HTML response
        httpd_resp_set_type(req, "text/html");
        httpd_resp_send(req, final_html, HTTPD_RESP_USE_STRLEN);

        free(modified_page);
        free(html_buffer);
        free(final_html);

        return ESP_OK;
    }

    // If no placeholder found, send the original file
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, modified_page, HTTPD_RESP_USE_STRLEN);

    free(modified_page);
    free(html_buffer);

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

    // Start the httpd server
    ESP_LOGI("WS", "Starting server on port: '%d'", config.server_port);

    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI("WS", "Registering URI handlers");

        httpd_uri_t login_uri = {
            .uri       = "/",
            .method    = HTTP_GET,
            .handler   = file_serve_handler,
            .user_ctx  = DEV ? "/templates/display.html" : "/templates/login.html"
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

        httpd_uri_t ws_uri = {
            .uri = "/browser_ws",
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

        httpd_uri_t connect_uri = {
            .uri       = "/connect",
            .method    = HTTP_GET,
            .handler   = file_serve_handler,
            .user_ctx  = "/templates/connect.html"
        };
        httpd_register_uri_handler(server, &connect_uri);

        httpd_uri_t eduroam_uri = {
            .uri       = "/eduroam",
            .method    = HTTP_GET,
            .handler   = file_serve_handler,
            .user_ctx  = "/templates/eduroam.html"
        };
        httpd_register_uri_handler(server, &eduroam_uri);

        httpd_uri_t validate_connect_uri = {
            .uri       = "/validate_connect",
            .method    = HTTP_POST,
            .handler   = validate_connect_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &validate_connect_uri);

        httpd_uri_t nearby_uri = {
            .uri       = "/nearby",
            .method    = HTTP_GET,
            .handler   = file_serve_handler,
            .user_ctx  = "/templates/nearby.html"
        };
        httpd_register_uri_handler(server, &nearby_uri);

        httpd_uri_t about_uri = {
            .uri       = "/about",
            .method    = HTTP_GET,
            .handler   = file_serve_handler,
            .user_ctx  = "/templates/about.html"
        };
        httpd_register_uri_handler(server, &about_uri);

        httpd_uri_t device_uri = {
            .uri       = "/device",
            .method    = HTTP_GET,
            .handler   = file_serve_handler,
            .user_ctx  = "/templates/device.html"
        };
        httpd_register_uri_handler(server, &device_uri);

        httpd_uri_t toggle_uri = {
            .uri = "/toggle",
            .method = HTTP_POST,
            .handler = toggle_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &toggle_uri);

        httpd_uri_t css_uri = {
            .uri      = "/static/portal/style.css",
            .method   = HTTP_GET,
            .handler  = file_serve_handler,
            .user_ctx = "/static/style.css",
        };
        httpd_register_uri_handler(server, &css_uri);

        httpd_uri_t image_uri = {
            .uri       = "/static/portal/images/aceon.png",
            .method    = HTTP_GET,
            .handler   = file_serve_handler,
            .user_ctx  = "/static/images/aceon.png"
        };
        httpd_register_uri_handler(server, &image_uri);

        httpd_uri_t image_uri_2 = {
            .uri       = "/static/portal/images/aceon2.png",
            .method    = HTTP_GET,
            .handler   = file_serve_handler,
            .user_ctx  = "/static/images/aceon2.png"
        };
        httpd_register_uri_handler(server, &image_uri_2);

    } else {
        ESP_LOGE("WS", "Error starting server!");
    }
    return server;
}

void check_wifi_task(void* pvParameters) {
    while(true) {
        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
            connected_to_WiFi = false;
        }

        check_bytes((TaskParams *)pvParameters);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void send_ws_message(const char *message) {
    if (xQueueSend(ws_queue, message, portMAX_DELAY) != pdPASS) {
        ESP_LOGE("WS", "WebSocket queue full! Dropping message: %s", message);
    }
}

void message_queue_task(void *pvParameters) {
    char message[WS_MESSAGE_MAX_LEN];

    while (true) {
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

            cJSON *typeItem = cJSON_GetObjectItem(message, "type");
            if (!typeItem) {
                ESP_LOGE("WS", "ilformatted json: %.*s", ws_event_data->data_len, (char *)ws_event_data->data_ptr);
                break;
            }
            const char *type = typeItem->valuestring;

            if (VERBOSE) ESP_LOGI("WS", "WebSocket data received: %.*s", ws_event_data->data_len, (char *)ws_event_data->data_ptr);

            if (strcmp(type, "query") == 0) {
                const char *content = cJSON_GetObjectItem(message, "content")->valuestring;
                if (strcmp(content, "are you still there?") == 0) {

                    cJSON *response_content = cJSON_CreateObject();
                    cJSON_AddStringToObject(response_content, "response", "yes");

                    cJSON *response = cJSON_CreateObject();
                    cJSON_AddStringToObject(response, "type", "response");
                    cJSON_AddStringToObject(response, "esp_id", ESP_ID);
                    cJSON_AddItemToObject(response, "content", response_content);

                    char *response_str = cJSON_PrintUnformatted(response);
                    cJSON_Delete(response);

                    xQueueReset(ws_queue);
                    send_ws_message(response_str);

                    free(response_str);
                }
            }

            else if (strcmp(type, "request") == 0) {
                cJSON *content = cJSON_GetObjectItem(message, "content");
                if (!content) {
                    ESP_LOGE("WS", "invalid request content");
                    cJSON_Delete(message);
                    break;
                }

                const char *endpoint = cJSON_GetObjectItem(content, "endpoint")->valuestring;
                const char *method = cJSON_GetObjectItem(content, "method")->valuestring;
                cJSON *data = cJSON_GetObjectItem(content, "data");

                httpd_req_t req = {
                    .uri = {*endpoint}
                };
                size_t req_len;
                char *req_content;
                esp_err_t err = ESP_OK;

                if ((strcmp(endpoint, "/validate_connect") == 0 || strcmp(endpoint, "/validate_connect?eduroam=true") == 0) && strcmp(method, "POST") == 0) {
                    reconnect = true;

                    // TODO: support eduroam

                    // create a mock HTTP request
                    cJSON *ssid = cJSON_GetObjectItem(data, "ssid");
                    cJSON *password = cJSON_GetObjectItem(data, "password");

                    req_len = snprintf(NULL, 0, "ssid=%s&password=%s", ssid->valuestring, password->valuestring);
                    req_content = malloc(req_len + 1);
                    snprintf(req_content, req_len + 1, "ssid=%s&password=%s", ssid->valuestring, password->valuestring);

                    req.content_len = req_len;
                    req.user_ctx = req_content;

                    // call validate_connect_handler
                    err = validate_connect_handler(&req);
                    if (err != ESP_OK) {
                        ESP_LOGE("WS", "Error in validate_connect_handler: %d", err);
                    }
                    free(req_content);
                }

                else if (strcmp(endpoint, "/validate_change") == 0 && strcmp(method, "POST") == 0) {
                    // create a mock HTTP request
                    cJSON *device_name = cJSON_GetObjectItem(data, "device_name");
                    cJSON *BL_voltage_threshold = cJSON_GetObjectItem(data, "BL_voltage_threshold");
                    cJSON *BH_voltage_threshold = cJSON_GetObjectItem(data, "BH_voltage_threshold");
                    cJSON *charge_current_threshold = cJSON_GetObjectItem(data, "charge_current_threshold");
                    cJSON *discharge_current_threshold = cJSON_GetObjectItem(data, "discharge_current_threshold");
                    cJSON *chg_inhibit_temp_low = cJSON_GetObjectItem(data, "chg_inhibit_temp_low");
                    cJSON *chg_inhibit_temp_high = cJSON_GetObjectItem(data, "chg_inhibit_temp_high");

                    req_len = snprintf(NULL, 0,
                        "device_name=%s&BL_voltage_threshold=%s&BH_voltage_threshold=%s&charge_current_threshold=%s&discharge_current_threshold=%s&chg_inhibit_temp_low=%s&chg_inhibit_temp_high=%s",
                        device_name->valuestring,
                        BL_voltage_threshold->valuestring,
                        BH_voltage_threshold->valuestring,
                        charge_current_threshold->valuestring,
                        discharge_current_threshold->valuestring,
                        chg_inhibit_temp_low->valuestring,
                        chg_inhibit_temp_high->valuestring);
                    req_content = malloc(req_len + 1);
                    snprintf(req_content, req_len + 1,
                        "device_name=%s&BL_voltage_threshold=%s&BH_voltage_threshold=%s&charge_current_threshold=%s&discharge_current_threshold=%s&chg_inhibit_temp_low=%s&chg_inhibit_temp_high=%s",
                        device_name->valuestring,
                        BL_voltage_threshold->valuestring,
                        BH_voltage_threshold->valuestring,
                        charge_current_threshold->valuestring,
                        discharge_current_threshold->valuestring,
                        chg_inhibit_temp_low->valuestring,
                        chg_inhibit_temp_high->valuestring);

                    req.content_len = req_len;
                    req.user_ctx = req_content;

                    // call validate_change_handler
                    err = validate_change_handler(&req);
                    if (err != ESP_OK) {
                        ESP_LOGE("WS", "Error in validate_change_handler: %d", err);
                    }
                    free(req_content);
                }

                else if (strcmp(endpoint, "/reset") == 0 && strcmp(method, "POST") == 0) {
                    // call reset_handler
                    err = reset_handler(&req);
                    if (err != ESP_OK) {
                        ESP_LOGE("WS", "Error in reset_handler: %d", err);
                    }
                }

                else if (strcmp(endpoint, "/toggle") == 0 && strcmp(method, "POST") == 0) {
                    // call toggle_handler
                    err = toggle_handler(&req);
                    if (err != ESP_OK) {
                        ESP_LOGE("WS", "Error in toggle_handler: %d", err);
                    }
                }

                cJSON *response_content = cJSON_CreateObject();
                cJSON_AddStringToObject(response_content, "endpoint", endpoint);
                cJSON_AddStringToObject(response_content, "status", err == ESP_OK ? "success" : "error");
                cJSON_AddNumberToObject(response_content, "error_code", err);
                cJSON_AddStringToObject(response_content, "response", req.user_ctx);

                cJSON *response = cJSON_CreateObject();
                cJSON_AddStringToObject(response, "type", "response");
                cJSON_AddStringToObject(response, "esp_id", ESP_ID);
                cJSON_AddItemToObject(response, "content", response_content);

                char *response_str = cJSON_PrintUnformatted(response);
                cJSON_Delete(response);

                send_ws_message(response_str);

                free(response_str);
            }

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
    while (true) {
        if (DEV) send_fake_post_request();

        // create JSON object with sensor data
        cJSON *json = cJSON_CreateObject();
        cJSON *data = cJSON_CreateObject();

        uint8_t eleven_bytes[11];
        uint8_t two_bytes[2];

        read_bytes(I2C_DATA_SUBCLASS_ID, I2C_NAME_OFFSET
            + 1 // what's this about????
        , eleven_bytes, sizeof(eleven_bytes));
        strncpy(ESP_ID, (char *)eleven_bytes, 10);
        cJSON_AddStringToObject(data, "name", ESP_ID);

        // read sensor data
        // these values are big-endian
        read_bytes(0, I2C_STATE_OF_CHARGE_REG, two_bytes, sizeof(two_bytes));
        cJSON_AddNumberToObject(data, "charge", two_bytes[1] << 8 | two_bytes[0]);
        read_bytes(0, I2C_VOLTAGE_REG, two_bytes, sizeof(two_bytes));
        cJSON_AddNumberToObject(data, "voltage", (float)(two_bytes[1] << 8 | two_bytes[0]) / 1000.0);
        read_bytes(0, I2C_CURRENT_REG, two_bytes, sizeof(two_bytes));
        cJSON_AddNumberToObject(data, "current", (float)((int16_t)(two_bytes[1] << 8 | two_bytes[0])) / 1000.0);
        // TODO:                                         ^check that this sorts the "two's complement"
        read_bytes(0, I2C_TEMPERATURE_REG, two_bytes, sizeof(two_bytes));
        cJSON_AddNumberToObject(data, "temperature", (float)(two_bytes[1] << 8 | two_bytes[0]) / 10.0 - 273.15);

        // configurable data too
        // these values are little-endian
        read_bytes(I2C_DISCHARGE_SUBCLASS_ID, I2C_BL_OFFSET, two_bytes, sizeof(two_bytes));
        cJSON_AddNumberToObject(data, "BL", two_bytes[0] << 8 | two_bytes[1]);
        read_bytes(I2C_DISCHARGE_SUBCLASS_ID, I2C_BH_OFFSET, two_bytes, sizeof(two_bytes));
        cJSON_AddNumberToObject(data, "BH", two_bytes[0] << 8 | two_bytes[1]);
        read_bytes(I2C_CURRENT_THRESHOLDS_SUBCLASS_ID, I2C_CHG_CURRENT_THRESHOLD_OFFSET, two_bytes, sizeof(two_bytes));
        cJSON_AddNumberToObject(data, "CCT", two_bytes[0] << 8 | two_bytes[1]);
        read_bytes(I2C_CURRENT_THRESHOLDS_SUBCLASS_ID, I2C_DSG_CURRENT_THRESHOLD_OFFSET, two_bytes, sizeof(two_bytes));
        cJSON_AddNumberToObject(data, "DCT", two_bytes[0] << 8 | two_bytes[1]);
        read_bytes(I2C_CHARGE_INHIBIT_CFG_SUBCLASS_ID, I2C_CHG_INHIBIT_TEMP_LOW_OFFSET, two_bytes, sizeof(two_bytes));
        cJSON_AddNumberToObject(data, "CITL", two_bytes[0] << 8 | two_bytes[1]);
        read_bytes(I2C_CHARGE_INHIBIT_CFG_SUBCLASS_ID, I2C_CHG_INHIBIT_TEMP_HIGH_OFFSET, two_bytes, sizeof(two_bytes));
        cJSON_AddNumberToObject(data, "CITH", two_bytes[0] << 8 | two_bytes[1]);

        cJSON_AddStringToObject(data, "IP", ESP_IP);
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

        check_bytes((TaskParams *)pvParameters);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
