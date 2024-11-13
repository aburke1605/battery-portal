#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_http_server.h>
#include <esp_spiffs.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <lwip/dns.h>

#include <string.h>

#include "DNS.c"
#include "I2C.c"
#include "html/index_page.h"
#include "html/display_page.h"

#define WIFI_SSID "ESP32-AP"
#define WIFI_PASS ""//"12345678"
#define MAX_STA_CONN 4

static int CONFIG_MAX_CLIENTS = 5;

void wifi_init_softap(void) {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize the Wi-Fi stack
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Configure the Access Point
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .channel = 1,
            .password = WIFI_PASS,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };

    if (strlen(WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP)); // Set ESP32 as AP
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI("test", "WiFi AP started. SSID: %s, Password: %s", WIFI_SSID, WIFI_PASS);
}

/*
 * Structure holding server handle
 * and internal socket fd in order
 * to use out of request send
 */
struct async_resp_arg {
    httpd_handle_t hd;
    int fd;
};

/*
 * async send function, which we put into the httpd work queue
 */
static void ws_async_send(void *arg) {
    static const char * data = "Async data";
    struct async_resp_arg *resp_arg = arg;
    httpd_handle_t hd = resp_arg->hd;
    int fd = resp_arg->fd;
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = (uint8_t*)data;
    ws_pkt.len = strlen(data);
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    httpd_ws_send_frame_async(hd, fd, &ws_pkt);
    free(resp_arg);
}

static esp_err_t trigger_async_send(httpd_handle_t handle, httpd_req_t *req) {
    struct async_resp_arg *resp_arg = malloc(sizeof(struct async_resp_arg));
    if (resp_arg == NULL) {
        return ESP_ERR_NO_MEM;
    }
    resp_arg->hd = req->handle;
    resp_arg->fd = httpd_req_to_sockfd(req);
    esp_err_t ret = httpd_queue_work(handle, ws_async_send, resp_arg);
    if (ret != ESP_OK) {
        free(resp_arg);
    }
    return ret;
}

/*
 * This handler echos back the received ws data
 * and triggers an async send if certain message received
 */
esp_err_t websocket_handler(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        ESP_LOGI("test", "Handshake done, the new connection was opened");
        return ESP_OK;
    }
    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    /* Set max_len = 0 to get the frame len */
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGE("test", "httpd_ws_recv_frame failed to get frame len with %d", ret);
        return ret;
    }
    ESP_LOGI("test", "frame len is %d", ws_pkt.len);
    if (ws_pkt.len) {
        /* ws_pkt.len + 1 is for NULL termination as we are expecting a string */
        buf = calloc(1, ws_pkt.len + 1);
        if (buf == NULL) {
            ESP_LOGE("test", "Failed to calloc memory for buf");
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        /* Set max_len = ws_pkt.len to get the frame payload */
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK) {
            ESP_LOGE("test", "httpd_ws_recv_frame failed with %d", ret);
            free(buf);
            return ret;
        }
        ESP_LOGI("test", "Got packet with message: %s", ws_pkt.payload);
    }
    ESP_LOGI("test", "Packet type: %d", ws_pkt.type);
    if (ws_pkt.type == HTTPD_WS_TYPE_TEXT &&
        strcmp((char*)ws_pkt.payload,"Trigger async") == 0) {
        free(buf);
        return trigger_async_send(req->handle, req);
    }

    ret = httpd_ws_send_frame(req, &ws_pkt);
    if (ret != ESP_OK) {
        ESP_LOGE("test", "httpd_ws_send_frame failed with %d", ret);
    }
    free(buf);
    return ret;
}

// Test websocket
esp_err_t test_websocket_handler(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        ESP_LOGI("test", "Handshake done, new connection opened");
    }

    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    /* Get the frame length */
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGE("test", "httpd_ws_recv_frame failed to get frame len with %d", ret);
        return ret;
    }
    ESP_LOGI("test", "Frame len is %d", ws_pkt.len);

    /* Receive payload if available */
    if (ws_pkt.len) {
        buf = calloc(1, ws_pkt.len + 1);
        if (buf == NULL) {
            ESP_LOGE("test", "Failed to calloc memory for buf");
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK) {
            ESP_LOGE("test", "httpd_ws_recv_frame failed with %d", ret);
            free(buf);
            return ret;
        }
        ESP_LOGI("test", "Received message: %s", ws_pkt.payload);
    }

    // if (ws_pkt.type == HTTPD_WS_TYPE_TEXT && strcmp((char *)ws_pkt.payload, "Trigger async") == 0) {
    //     free(buf);
    //     return trigger_async_send(req->handle, req);
    // }

    // /* Send echo message back */
    // ret = httpd_ws_send_frame(req, &ws_pkt);
    // if (ret != ESP_OK) {
    //     ESP_LOGE("test", "httpd_ws_send_frame failed with %d", ret);
    // }

    // free(buf);

    char *response_msg = "Hello from ESP32!";
    httpd_ws_frame_t ws_response;
    memset(&ws_response, 0, sizeof(httpd_ws_frame_t));
    ws_response.type = HTTPD_WS_TYPE_TEXT;
    ws_response.payload = (uint8_t *)response_msg;
    ws_response.len = strlen(response_msg);

    ret = httpd_ws_send_frame(req, &ws_response);
    if (ret != ESP_OK) {
        ESP_LOGE("test", "Failed to send message to client with error: %d", ret);
    } else {
        ESP_LOGI("test", "Message sent to client: %s", response_msg);
    }

    free(buf);

    return ret;
}

/* Function to periodically send a message */
void websocket_send_task(void *arg) {
    httpd_handle_t server = (httpd_handle_t)arg;
    httpd_ws_frame_t ws_pkt;
    char *message = "Hello from ESP32";

    while (true) {
        memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
        ws_pkt.type = HTTPD_WS_TYPE_TEXT;
        ws_pkt.payload = (uint8_t *)message;
        ws_pkt.len = strlen(message);

        /* Send the message to all connected clients */
        for (int fd = 0; fd < CONFIG_MAX_CLIENTS; fd++) {
            esp_err_t ret = httpd_ws_send_frame_async(server, fd, &ws_pkt);
            if (ret == ESP_OK) {
                ESP_LOGI("test", "Sent message to client %d", fd);
            } else {
                ESP_LOGE("test", "Failed to send message to client %d, error %d", fd, ret);
            }
        }

        /* Wait for 5 seconds */
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// Configure URI for WebSocket connection
static const httpd_uri_t ws = {
        .uri        = "/ws", // URI to establish WebSocket connection
        .method     = HTTP_GET,
        .handler    = test_websocket_handler,
        .user_ctx   = NULL,
        .is_websocket = true // Mark this URI as WebSocket
};

// Function to handle HTTP GET requests
esp_err_t index_handler(httpd_req_t *req) {
    httpd_resp_send(req, index_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Function to parse parameters from POST data (simple key=value parsing)
void parse_post_data(const char *data, char *username, char *password) {
    // Search for "username=" and "password=" in the data
    const char *user_pos = strstr(data, "username=");
    const char *pass_pos = strstr(data, "password=");
    
    if (user_pos) {
        user_pos += strlen("username=");
        sscanf(user_pos, "%[^&]", username); // Copy up to '&' character
    }

    if (pass_pos) {
        pass_pos += strlen("password=");
        sscanf(pass_pos, "%[^&]", password); // Copy up to '&' character
    }
}

// Function to handle HTTP GET requests
esp_err_t display_handler(httpd_req_t *req) {
    // Define buffer to receive POST data
    char content[100];
    char username[50] = {0}; // Buffer for the username
    char password[50] = {0}; // Buffer for the password

    // Get the length of content in the request
    int content_len = req->content_len;
    if (content_len >= sizeof(content)) {
        httpd_resp_send_500(req); // Return error if content is too large
        return ESP_FAIL;
    }

    // Receive the data into the buffer
    int received = httpd_req_recv(req, content, content_len);
    if (received <= 0) {
        httpd_resp_send_500(req); // Error in receiving data
        return ESP_FAIL;
    }
    
    // Null-terminate the received data
    content[received] = '\0';

    // Parse the POST data for username and password
    parse_post_data(content, username, password);

    ESP_LOGI("test", "Received query string: %s", username);

    ESP_LOGI("test", "Received query string: %s", password);

    if (strcmp(username, "admin") == 0 && strcmp(password, "1234") == 0) 
    {
        httpd_resp_send(req, display_html, HTTPD_RESP_USE_STRLEN);
    } else {
        const char *error_html = "<html><body><h2>Login Failed</h2><p>Invalid username or password.</p></body></html>";
        httpd_resp_send(req, error_html, HTTPD_RESP_USE_STRLEN);
    }
    return ESP_OK;
}

// TODO dress this function so that the same one can be used for aceon and aceon2
esp_err_t image_get_handler(httpd_req_t *req) { //, const char *file_path) {
    // Path to the file in the SPIFFS partition
    const char *file_path = "/storage/aceon.png";
    FILE *file = fopen(file_path, "r");

    if (file == NULL) {
        ESP_LOGE("SPIFFS", "Failed to open file: %s", file_path);
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
            ESP_LOGE("SPIFFS", "Failed to send file chunk");
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
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI("test", "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI("test", "Registering URI handlers");
        httpd_register_uri_handler(server, &ws);  // Register WebSocket URI

        // Register ios detect page
        httpd_uri_t hotspot_detect_get_ios = {
            .uri       = "/hotspot-detect.html",
            .method    = HTTP_GET,
            .handler   = index_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &hotspot_detect_get_ios);

        // Register android detect page
        httpd_uri_t hotspot_detect_get_android = {
            .uri       = "/generate_204",
            .method    = HTTP_GET,
            .handler   = index_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &hotspot_detect_get_android);

        // Register windows detect page
        httpd_uri_t hotspot_detect_get_windows = {
            .uri       = "/connecttest.txt",
            .method    = HTTP_GET,
            .handler   = index_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &hotspot_detect_get_windows);

        httpd_uri_t hotspot_detect_get_windows2 = {
            .uri       = "/favicon.ico",
            .method    = HTTP_GET,
            .handler   = index_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &hotspot_detect_get_windows2);

        httpd_uri_t hotspot_detect_get_windows3 = {
            .uri       = "/redirect",
            .method    = HTTP_GET,
            .handler   = index_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &hotspot_detect_get_windows3);

        // Display page
        httpd_uri_t display_page_get = {
            .uri       = "/display",
            .method    = HTTP_POST,
            .handler   = display_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &display_page_get);

        // Add a handler for serving the image
        httpd_uri_t image_uri = {
            .uri       = "/image/aceon.png",
            .method    = HTTP_GET,
            .handler   = image_get_handler, // Function to read and send the image
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &image_uri);

        return server;
    }

    ESP_LOGI("test", "Error starting server!");
    return NULL;
}

void app_main(void) {

    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI("test", "I2C initialized successfully");

    uint16_t iCharge = read_2byte_data(STATE_OF_CHARGE_REG);
    ESP_LOGI("test", "Charge: %d %%", iCharge);

    uint16_t iVoltage = read_2byte_data(VOLTAGE_REG);
    float fVoltage = (float)iVoltage / 1000.0;
    ESP_LOGI("test", "Voltage: %.2f V", fVoltage);

    uint16_t iCurrent = read_2byte_data(CURRENT_REG);
    float fCurrent = (float)iCurrent / 1000.0;
    ESP_LOGI("test", "Current: %.2f A", fCurrent);

    uint16_t iTemperature = read_2byte_data(TEMPERATURE_REG);
    float fTemperature = (float)iTemperature / 10.0 - 273.15;
    ESP_LOGI("test", "Temperature: %.2f \u00B0C", fTemperature);

    return;

    // Start the Access Point
    wifi_init_softap();

    // Start the ws server to serve the HTML page
    static httpd_handle_t server = NULL;
    server =  start_webserver();

    // Start DNS server task
    xTaskCreate(&dns_server_task, "dns_server_task", 4096, NULL, 5, NULL);

    // xTaskCreate(&websocket_send_task, "websocket_send_task", 4096, server, 5, NULL);
}
