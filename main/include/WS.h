#include <esp_log.h>
#include <esp_http_server.h>
#include <esp_websocket_client.h>
#include <cJSON.h>
#include <lwip/sockets.h>
#include <driver/gpio.h>


#define CONFIG_MAX_CLIENTS 5
#define LED_GPIO_PIN 2

extern httpd_handle_t server;
extern int client_sockets[CONFIG_MAX_CLIENTS];
extern char received_data[1024];
extern SemaphoreHandle_t data_mutex;
extern bool connected_to_WiFi;

void add_client(int fd);

void remove_client(int fd);

esp_err_t login_handler(httpd_req_t *req);

esp_err_t validate_login_handler(httpd_req_t *req);

esp_err_t websocket_handler(httpd_req_t *req);

esp_err_t connect_handler(httpd_req_t *req);

esp_err_t validate_connect_handler(httpd_req_t *req);

esp_err_t nearby_handler(httpd_req_t *req);

esp_err_t about_handler(httpd_req_t *req);

esp_err_t device_handler(httpd_req_t *req);

esp_err_t toggle_handler(httpd_req_t *req);

esp_err_t css_handler(httpd_req_t *req);

esp_err_t image_handler(httpd_req_t *req);

esp_err_t file_serve_handler(httpd_req_t *req);

httpd_handle_t start_webserver(void);

void websocket_broadcast_task(void *pvParameters);

void website_send_task(void *pvParameters);
