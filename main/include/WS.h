#include <esp_log.h>
#include <esp_http_server.h>
#include <esp_websocket_client.h>
#include <cJSON.h>
#include <lwip/sockets.h>
#include <driver/gpio.h>


#define CONFIG_MAX_CLIENTS 5

extern httpd_handle_t server;
extern int client_sockets[CONFIG_MAX_CLIENTS];
extern char received_data[1024];
extern SemaphoreHandle_t data_mutex;
extern bool connected_to_WiFi;
extern char successful_ips[256][16];
extern char old_successful_ips[256][16];
extern uint8_t successful_ip_count;
extern uint8_t old_successful_ip_count;
extern char ESP_IP[16];

void add_client(int fd);

void remove_client(int fd);

esp_err_t validate_login_handler(httpd_req_t *req);

esp_err_t websocket_handler(httpd_req_t *req);

esp_err_t validate_change_handler(httpd_req_t *req);

esp_err_t reset_handler(httpd_req_t *req);

esp_err_t validate_connect_handler(httpd_req_t *req);

esp_err_t toggle_handler(httpd_req_t *req);

esp_err_t css_handler(httpd_req_t *req);

esp_err_t image_handler(httpd_req_t *req);

esp_err_t file_serve_handler(httpd_req_t *req);

httpd_handle_t start_webserver(void);

void web_task(void *pvParameters);

void check_wifi_task(void* pvParameters);
