#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_server.h"

#include <string.h>

#include "html.h"

#define WIFI_SSID "ESP32-AP"
#define WIFI_PASS "12345678"
#define MAX_STA_CONN 4

static const char *TAG = "wifi_softAP";

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

    if (strlen(WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP)); // Set ESP32 as AP
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi AP started. SSID: %s, Password: %s", WIFI_SSID, WIFI_PASS);
}

// Function to handle HTTP GET requests
esp_err_t index_handler(httpd_req_t *req)
{
    httpd_resp_send(req, index_html, HTTPD_RESP_USE_STRLEN); // Send the HTML page as a response
    return ESP_OK;
}

// Start HTTP server
httpd_handle_t start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        // Register URI handler
        httpd_uri_t uri_get = {
            .uri       = "/",
            .method    = HTTP_GET,
            .handler   = index_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &uri_get);
    }
    return server;
}

void app_main(void)
{
    // Start the Access Point
    wifi_init_softap();

    // Start the HTTP server to serve the HTML page
    start_webserver();
}
