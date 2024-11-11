#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_http_server.h>
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include <string.h>

#include "html.h"

#define WIFI_SSID "ESP32-AP-TEST"
#define WIFI_PASS "12345678"
#define MAX_STA_CONN 4
#define DNS_PORT 53

static const char *TAG = "websocket_server";

void wifi_init_softap(void)
{
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

    //if (strlen(WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    //}

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP)); // Set ESP32 as AP
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi AP started. SSID: %s, Password: %s", WIFI_SSID, WIFI_PASS);
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
static void ws_async_send(void *arg)
{
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

static esp_err_t trigger_async_send(httpd_handle_t handle, httpd_req_t *req)
{
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
esp_err_t websocket_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "Handshake done, the new connection was opened");
        return ESP_OK;
    }
    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    /* Set max_len = 0 to get the frame len */
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %d", ret);
        return ret;
    }
    ESP_LOGI(TAG, "frame len is %d", ws_pkt.len);
    if (ws_pkt.len) {
        /* ws_pkt.len + 1 is for NULL termination as we are expecting a string */
        buf = calloc(1, ws_pkt.len + 1);
        if (buf == NULL) {
            ESP_LOGE(TAG, "Failed to calloc memory for buf");
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        /* Set max_len = ws_pkt.len to get the frame payload */
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
            free(buf);
            return ret;
        }
        ESP_LOGI(TAG, "Got packet with message: %s", ws_pkt.payload);
    }
    ESP_LOGI(TAG, "Packet type: %d", ws_pkt.type);
    if (ws_pkt.type == HTTPD_WS_TYPE_TEXT &&
        strcmp((char*)ws_pkt.payload,"Trigger async") == 0) {
        free(buf);
        return trigger_async_send(req->handle, req);
    }

    ret = httpd_ws_send_frame(req, &ws_pkt);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_send_frame failed with %d", ret);
    }
    free(buf);
    return ret;
}


// Configure URI for WebSocket connection
static const httpd_uri_t ws = {
        .uri        = "/ws", // URI to establish WebSocket connection
        .method     = HTTP_GET,
        .handler    = websocket_handler,
        .user_ctx   = NULL,
        .is_websocket = true // Mark this URI as WebSocket
};

// TODO Function to handle HTTP GET requests  handle all the request
esp_err_t index_handler(httpd_req_t *req)
{
    httpd_resp_send(req, index_html, HTTPD_RESP_USE_STRLEN); // Send the HTML page as a response
    return ESP_OK;
}

// Start HTTP server
httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Registering URI handlers");
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


        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

// DNS handler (redirect all requests to our AP IP)
static void dns_server_task(void *pvParameters) {
    struct sockaddr_in dest_addr;
    struct sockaddr_in source_addr;
    uint8_t buffer[512];
    int addr_family = AF_INET;
    int ip_protocol = IPPROTO_IP;

    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(DNS_PORT);

    // Create UDP socket
    int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
    }
    if (bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        close(sock);
        vTaskDelete(NULL);
    }

    ESP_LOGI(TAG, "DNS server started on port %d", DNS_PORT);

    // Get AP IP address
    esp_netif_ip_info_t ip_info;
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if (netif == NULL || esp_netif_get_ip_info(netif, &ip_info) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get AP IP address");
        vTaskDelete(NULL);
    }

    while (1) {
        socklen_t socklen = sizeof(source_addr);
        int len = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&source_addr, &socklen);

        if (len > 0) {
            ESP_LOGI(TAG, "DNS request received, responding with AP IP: " IPSTR, IP2STR(&ip_info.ip));

            // Set the DNS response flags
            buffer[2] = 0x81; // Response flag and authoritative answer
            buffer[3] = 0x80; // Recursion available

            // Questions count (already in the request, copying as is)
            // Answer count: Set to 1
            buffer[6] = 0x00;
            buffer[7] = 0x01;

            // Append answer record after the question
            int response_offset = len;

            // Point to the answer section, matching the question section
            buffer[response_offset++] = 0xc0; // Pointer to name offset
            buffer[response_offset++] = 0x0c;

            // Type A record (IPv4 address)
            buffer[response_offset++] = 0x00;
            buffer[response_offset++] = 0x01;

            // Class IN
            buffer[response_offset++] = 0x00;
            buffer[response_offset++] = 0x01;

            // TTL (Time to live) - 5 minutes
            buffer[response_offset++] = 0x00;
            buffer[response_offset++] = 0x00;
            buffer[response_offset++] = 0x0e;
            buffer[response_offset++] = 0x10;

            // Data length (IPv4 address length)
            buffer[response_offset++] = 0x00;
            buffer[response_offset++] = 0x04;

            // Add the AP IP address as the answer (4 bytes)
            buffer[response_offset++] = ip4_addr1(&ip_info.ip);
            buffer[response_offset++] = ip4_addr2(&ip_info.ip);
            buffer[response_offset++] = ip4_addr3(&ip_info.ip);
            buffer[response_offset++] = ip4_addr4(&ip_info.ip);

            // Send DNS response
            sendto(sock, buffer, response_offset, 0, (struct sockaddr *)&source_addr, socklen);
        } else {
            ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
        }
    }

    // Cleanup if the loop exits
    close(sock);
    vTaskDelete(NULL);
}

void app_main(void)
{
    // Start the Access Point
    wifi_init_softap();

    // Start the ws server to serve the HTML page
    static httpd_handle_t server = NULL;
    server =  start_webserver();

    // Start DNS server task
    xTaskCreate(&dns_server_task, "dns_server_task", 4096, NULL, 5, NULL);
}
