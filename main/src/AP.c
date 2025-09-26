#include "include/AP.h"

#include "include/global.h"
#include "include/config.h"
#include "include/I2C.h"
#include "include/WS.h"
#include "include/utils.h"

#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_wifi.h>
#include <esp_mac.h>
#include <esp_websocket_client.h>

static struct rendered_page rendered_html_pages[WS_MAX_N_HTML_PAGES];
static uint8_t n_rendered_html_pages = 0;

static const char* TAG = "AP";

wifi_ap_record_t *wifi_scan(void) {
    // configure Wi-Fi scan settings
    wifi_scan_config_t scan_config = {
        .ssid = NULL,        // all SSIDs
        .bssid = NULL,       // all BSSIDs
        .channel = 0,        // all channels
        .show_hidden = false,
        .scan_type = WIFI_SCAN_TYPE_PASSIVE,
    };

    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true)); // blocking scan

    // get the number of APs found
    uint16_t ap_num = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_num));

    // allocate memory for AP info and retrieve the list
    wifi_ap_record_t *ap_info = malloc(sizeof(wifi_ap_record_t) * ap_num);
    if (ap_info == NULL) {
        ESP_LOGE("AP", "Failed to allocate memory for AP list");
        return false;
    }
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_num, ap_info));

    for (int i = 0; i < ap_num; i++) {
        if (strncmp((const char*)ap_info[i].ssid, "ROOT ", 5) == 0) return &ap_info[i];
    }

    free(ap_info);

    return NULL;
}

void ap_n_clients_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "Wi-Fi STA started");
                break;

            case WIFI_EVENT_AP_STACONNECTED: {
                wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *) event_data;
                num_connected_clients++;
                ESP_LOGI(TAG, "Station "MACSTR" joined, AID=%d. Clients: %d",
                         MAC2STR(event->mac), event->aid, num_connected_clients);
                break;
            }

            case WIFI_EVENT_AP_STADISCONNECTED: {
                wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *) event_data;
                num_connected_clients--;
                ESP_LOGI(TAG, "Station "MACSTR" left, AID=%d. Clients: %d",
                         MAC2STR(event->mac), event->aid, num_connected_clients);
                break;
            }

            default:
                break;
        }
    }
}

void wifi_init(void) {
    // initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    // initialize the Wi-Fi stack
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // use only STA at first to scan for APs
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    vTaskDelay(pdMS_TO_TICKS(100));

    // scan for other WiFi APs
    wifi_ap_record_t * AP_exists = wifi_scan();

    // stop WiFi before changing mode
    ESP_ERROR_CHECK(esp_wifi_stop());

    // create the AP network interface
    ap_netif = esp_netif_create_default_wifi_ap();

    // set Wi-Fi mode to both AP and STA
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    // configure the Access Point
    wifi_config_t wifi_ap_config = {
        .ap = {
            .channel = 1,
            .max_connection = AP_MAX_STA_CONN,
            .authmode = WIFI_AUTH_OPEN
        },
    };

    // set the SSID as well
    char buffer[5 + 8 + 2 + 5 + 1 + 1]; // "ROOT " + "BMS_255" + ": " + uint16_t, + "%" + "\0"
    if (LORA_IS_RECEIVER) {
        snprintf(buffer, sizeof(buffer), "LoRa RECEIVER");
    } else {
        uint8_t data_SBS[2] = {0};
        read_SBS_data(I2C_RELATIVE_STATE_OF_CHARGE_ADDR, data_SBS, sizeof(data_SBS));
        snprintf(buffer, sizeof(buffer), "%sbms_%02u: %d%%", !AP_exists?"ROOT ":"", ESP_ID, data_SBS[1] << 8 | data_SBS[0]);
    }

    strncpy((char *)wifi_ap_config.ap.ssid, buffer, sizeof(wifi_ap_config.ap.ssid) - 1);
    wifi_ap_config.ap.ssid[sizeof(wifi_ap_config.ap.ssid) - 1] = '\0'; // ensure null-termination
    wifi_ap_config.ap.ssid_len = strlen((char *)wifi_ap_config.ap.ssid); // set SSID length

    // specific configurations or ROOT or non-ROOT APs
    if (!AP_exists) {
        // keep count of number of connected clients
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &ap_n_clients_handler, NULL));
        is_root = true;
    } else {
        // must change IP address from default so
        // can send messages to ROOT at 192.168.4.1
        esp_netif_ip_info_t ip_info;
        IP4_ADDR(&ip_info.ip, 192,168,5,1);
        IP4_ADDR(&ip_info.gw, 192,168,5,1);
        IP4_ADDR(&ip_info.netmask, 255,255,255,0);
        esp_netif_dhcps_stop(ap_netif);
        esp_netif_set_ip_info(ap_netif, &ip_info);
        esp_netif_dhcps_start(ap_netif);
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_ap_config));

    // restart WiFi
    ESP_LOGI(TAG, "Starting WiFi AP... SSID: %s", wifi_ap_config.ap.ssid);
    ESP_ERROR_CHECK(esp_wifi_start());
    vTaskDelay(pdMS_TO_TICKS(100));
}

esp_err_t redirect_handler(httpd_req_t *req) {
    // just need to redirect to the uri where ws data is sent
    char redirect_url[35];
    snprintf(redirect_url, sizeof(redirect_url), "/esp32?esp_id=%u", ESP_ID);

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

esp_err_t login_handler(httpd_req_t *req) {
    char response[39 + UTILS_AUTH_TOKEN_LENGTH];
    httpd_resp_set_type(req, "application/json");

    char *check_session = strstr(req->uri, "/api/user/check-auth?auth_token=");
    if (check_session) {
        char auth_token[UTILS_AUTH_TOKEN_LENGTH] = {0};
        sscanf(check_session,"/api/user/check-auth?auth_token=%50s", auth_token);

        for (int i=0; i < WS_CONFIG_MAX_CLIENTS; i++) {
            if (strcmp(client_sockets[i].auth_token, auth_token) == 0) {
                httpd_resp_set_status(req, HTTPD_200);
                strncpy(current_auth_token, auth_token, UTILS_AUTH_TOKEN_LENGTH);
                current_auth_token[UTILS_AUTH_TOKEN_LENGTH - 1] = '\0';
                snprintf(response, sizeof(response), "{\"loggedIn\": true, \"email\": \"%s\"}", WS_USERNAME);
                httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
                return ESP_OK;
            }
        }
        httpd_resp_set_status(req, "401 Unauthorized");
        strcpy(response, "{\"loggedIn\": false}");
        httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }

    // else its the standard login request
    char content[100];
    esp_err_t err = get_POST_data(req, content, sizeof(content));
    if (err != ESP_OK) {
        ESP_LOGE("WS", "Problem with login POST request");
        return err;
    }

    cJSON *credentials = cJSON_Parse(content);
    if (!credentials) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return ESP_FAIL;
    }

    cJSON *email = cJSON_GetObjectItem(credentials, "email");
    cJSON *password = cJSON_GetObjectItem(credentials, "password");
    if (!email || !password) {
        ESP_LOGE(TAG, "Error in POST content");
        return ESP_FAIL;
    }

    char email_decoded[50] = {0};
    char password_decoded[50] = {0};
    url_decode(email_decoded, email->valuestring);
    url_decode(password_decoded, password->valuestring);

    if (DEV || (strcmp(email_decoded, WS_USERNAME) == 0 && strcmp(password_decoded, WS_PASSWORD) == 0)) {
        httpd_resp_set_status(req, HTTPD_200);
        random_token(current_auth_token);
        current_auth_token[UTILS_AUTH_TOKEN_LENGTH - 1] = '\0';
        snprintf(response, sizeof(response), "{\"success\": true, \"auth_token\": \"%s\"}", current_auth_token);
    } else {
        httpd_resp_set_status(req, "401 Unauthorized");
        strcpy(response, "{\"success\": false}");
    }
    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t num_clients_handler(httpd_req_t *req) {
    vTaskSuspend(merge_root_task_handle);
    char response[64];
    snprintf(response, sizeof(response), "{\"num_connected_clients\": %d}", num_connected_clients);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);

    // big delay before resuming to give
    // other AP time to receive message
    vTaskDelay(pdMS_TO_TICKS(10000));
    vTaskResume(merge_root_task_handle);

    return ESP_OK;
}

esp_err_t restart_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "I am being told to restart");
    esp_restart();

    return ESP_OK;
}

httpd_handle_t start_webserver(void) {
    // create sockets for clients
    for (int i = 0; i < WS_CONFIG_MAX_CLIENTS; i++) {
        client_sockets[i].descriptor = -1;
        client_sockets[i].auth_token[0] = '\0';
        client_sockets[i].is_browser_not_mesh = true;
        client_sockets[i].esp_id = 0;
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

        httpd_uri_t login_uri = {
            .uri       = "/api/user/login",
            .method    = HTTP_POST,
            .handler   = login_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &login_uri);

        httpd_uri_t refresh_uri = {
            .uri       = "/api/user/check-auth",
            .method    = HTTP_GET,
            .handler   = login_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &refresh_uri);

        httpd_uri_t ws_uri = {
            .uri = "/api/browser_ws",
            .method = HTTP_GET,
            .handler = client_handler,
            .is_websocket = true
        };
        httpd_register_uri_handler(server, &ws_uri);

        ws_uri.uri = "/mesh_ws";
        httpd_register_uri_handler(server, &ws_uri);

        httpd_uri_t mesh_uri = {
            .uri       = "/api_num_clients",
            .method    = HTTP_GET,
            .handler   = num_clients_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &mesh_uri);

        mesh_uri.uri = "/no_you_restart",
        mesh_uri.method = HTTP_POST,
        mesh_uri.handler = restart_handler,
        httpd_register_uri_handler(server, &mesh_uri);

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

        js_uri.uri = "/assets/AuthRequire.js";
        js_uri.user_ctx = "/static/assets/AuthRequire.js";
        httpd_register_uri_handler(server, &js_uri);

        httpd_uri_t css_uri = {
            .uri      = "/assets/AuthRequire.css",
            .method   = HTTP_GET,
            .handler  = file_serve_handler,
            .user_ctx = "/static/assets/AuthRequire.css",
        };
        httpd_register_uri_handler(server, &css_uri);

    } else {
        ESP_LOGE(TAG, "Error starting server!");
    }
    return server;
}
