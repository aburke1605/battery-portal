#include "include/AP.h"

#include "include/global.h"
#include "include/config.h"
#include "include/I2C.h"
#include "include/WS.h"
#include "include/utils.h"

#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_wifi.h>
#include <esp_websocket_client.h>

static struct rendered_page rendered_html_pages[WS_MAX_N_HTML_PAGES];
static uint8_t n_rendered_html_pages = 0;

static const char* TAG = "AP";

void wifi_init(void) {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    // Initialize the Wi-Fi stack
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    esp_netif_create_default_wifi_sta();
    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // scan for avail wifi networks first
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_ERROR_CHECK(esp_wifi_stop());

    // Set Wi-Fi mode to both AP and STA
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    // Configure the Access Point
    wifi_config_t wifi_ap_config = {
        .ap = {
            .channel = 1,
            .max_connection = AP_MAX_STA_CONN,
            .authmode = WIFI_AUTH_OPEN
        },
    };

    // set the SSID as well
    uint8_t name[11];
    read_bytes(I2C_DATA_SUBCLASS_ID, I2C_NAME_OFFSET, name, sizeof(name));
    strncpy(ESP_ID, (char *)name, 10);

    uint8_t charge[2] = {};
    read_bytes(0, I2C_STATE_OF_CHARGE_REG, charge, sizeof(charge));

    char buffer[strlen(ESP_ID) + 2 + 5 + 1 + 1]; // "BMS_01" + ": " + uint16_t, + "%" + "\0"
    snprintf(buffer, sizeof(buffer), "%s: %d%%", ESP_ID, charge[1] << 8 | charge[0]);

    strncpy((char *)wifi_ap_config.ap.ssid, buffer, sizeof(wifi_ap_config.ap.ssid) - 1);
    wifi_ap_config.ap.ssid[sizeof(wifi_ap_config.ap.ssid) - 1] = '\0'; // Ensure null-termination
    wifi_ap_config.ap.ssid_len = strlen((char *)wifi_ap_config.ap.ssid); // Set SSID length

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_ap_config));


    // manually set the netmask and gateway as something different from first ESP
    ESP_ERROR_CHECK(esp_netif_dhcps_stop(ap_netif));
    snprintf(ESP_subnet_IP, sizeof(ESP_subnet_IP), "192.168.4.1");
    esp_ip4_addr_t ip_info = {
        .addr = esp_ip4addr_aton(ESP_subnet_IP),
    };
    esp_ip4_addr_t netmask = {
        .addr = esp_ip4addr_aton("255.255.255.0"),
    };

    esp_ip4_addr_t gateway = {
        .addr = esp_ip4addr_aton(ESP_subnet_IP),
    };
    esp_netif_ip_info_t ip_info_struct;
    ip_info_struct.ip = ip_info;
    ip_info_struct.netmask = netmask;
    ip_info_struct.gw = gateway;
    ESP_ERROR_CHECK(esp_netif_set_ip_info(ap_netif, &ip_info_struct));
    ESP_ERROR_CHECK(esp_netif_dhcps_start(ap_netif));
    ESP_LOGI(TAG, "AP initialized with IP: %s", ESP_subnet_IP);

    ESP_LOGI(TAG, "Starting WiFi AP... SSID: %s", wifi_ap_config.ap.ssid);
    ESP_ERROR_CHECK(esp_wifi_start());
}

esp_err_t redirect_handler(httpd_req_t *req) {
    // just need to redirect to the uri where ws data is sent
    char redirect_url[25];
    snprintf(redirect_url, sizeof(redirect_url), "/esp32?esp_id=%s", ESP_ID);

    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", redirect_url);
    httpd_resp_send(req, NULL, 0); // no response body

    return ESP_OK;
}

esp_err_t file_serve_handler(httpd_req_t *req) {
    const char *file_path = (const char *)req->user_ctx;
    if (VERBOSE) ESP_LOGI(TAG, "Serving file: %s", file_path);

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
            ESP_LOGE(TAG, "Failed to serve HTML file");
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found");
            return ESP_FAIL;
        }

        struct rendered_page* new_page = malloc(sizeof(struct rendered_page));
        if (!new_page) {
            ESP_LOGE(TAG, "Couldn't allocate memory in heap!");
            return ESP_FAIL;
        }

        strncpy(new_page->name, file_path, WS_MAX_HTML_PAGE_NAME_LENGTH - 1);
        new_page->name[WS_MAX_HTML_PAGE_NAME_LENGTH - 1] = '\0';

        strncpy(new_page->content, content_html, WS_MAX_HTML_SIZE - 1);
        new_page->content[WS_MAX_HTML_SIZE - 1] = '\0';

        rendered_html_pages[n_rendered_html_pages++] = *new_page;
        free(new_page);

        httpd_resp_set_type(req, "text/html");
        httpd_resp_send(req, content_html, HTTPD_RESP_USE_STRLEN);

        free(content_html);

        return ESP_OK;
    }


    // for non-HTML files, serve normally
    FILE *file = fopen(file_path, "r");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open file: %s", file_path);
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found");
        return ESP_FAIL;
    }

    // Get file size
    struct stat file_stat;
    if (stat(file_path, &file_stat) != 0) {
        ESP_LOGE(TAG, "Failed to get file stats: %s", file_path);
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
            ESP_LOGE(TAG, "Failed to send file chunk");
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
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);

    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Registering URI handlers");

        httpd_uri_t redirect_uri = {
            .uri       = "/",
            .method    = HTTP_GET,
            .handler   = redirect_handler,
        };
        httpd_register_uri_handler(server, &redirect_uri);

        // do some more to catch all device types
        redirect_uri.uri = "/hotspot-detect.html";
        httpd_register_uri_handler(server, &redirect_uri);
        redirect_uri.uri = "/generate_204";
        httpd_register_uri_handler(server, &redirect_uri);
        redirect_uri.uri = "/connecttest.txt";
        httpd_register_uri_handler(server, &redirect_uri);
        redirect_uri.uri = "/favicon.ico";
        httpd_register_uri_handler(server, &redirect_uri);
        redirect_uri.uri = "/redirect";
        httpd_register_uri_handler(server, &redirect_uri);
        redirect_uri.uri = "/wpad.dat";
        httpd_register_uri_handler(server, &redirect_uri);
        redirect_uri.uri = "/gen_204";
        httpd_register_uri_handler(server, &redirect_uri);

        httpd_uri_t esp32_uri = {
            .uri       = "/esp32",
            .method    = HTTP_GET,
            .handler   = file_serve_handler,
            .user_ctx  = "/static/esp32.html"
        };
        httpd_register_uri_handler(server, &esp32_uri);

        httpd_uri_t ws_uri = {
            .uri = "/browser_ws",
            .method = HTTP_GET,
            .handler = client_handler,
            .is_websocket = true
        };
        httpd_register_uri_handler(server, &ws_uri);

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
        ESP_LOGE(TAG, "Error starting server!");
    }
    return server;
}
