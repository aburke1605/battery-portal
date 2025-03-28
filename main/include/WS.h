#include <esp_log.h>
#include <esp_http_server.h>
#include <esp_websocket_client.h>
#include <cJSON.h>
#include <lwip/sockets.h>
#include <driver/gpio.h>

#include "include/config.h"
#include "include/utils.h"

struct rendered_page {
    char name[WS_MAX_HTML_PAGE_NAME_LENGTH];
    char content[WS_MAX_HTML_SIZE];
};

extern char ESP_ID[UTILS_KEY_LENGTH + 1];
extern httpd_handle_t server;
extern int client_sockets[WS_CONFIG_MAX_CLIENTS];
extern char received_data[1024];
extern SemaphoreHandle_t data_mutex;
extern bool connected_to_WiFi;
extern bool reconnect;
extern char successful_ips[256][16];
extern char old_successful_ips[256][16];
extern uint8_t successful_ip_count;
extern uint8_t old_successful_ip_count;
extern char ESP_IP[16];
extern esp_websocket_client_handle_t ws_client;
extern QueueHandle_t ws_queue;
extern struct rendered_page rendered_html_pages[WS_MAX_N_HTML_PAGES];
extern uint8_t n_rendered_html_pages;
extern bool admin_verified;

void add_client(int fd);

void remove_client(int fd);

esp_err_t perform_request(cJSON *message, cJSON *response);

esp_err_t validate_login_handler(httpd_req_t *req);

esp_err_t websocket_handler(httpd_req_t *req);

esp_err_t validate_change_handler(httpd_req_t *req);

esp_err_t reset_handler(httpd_req_t *req);

esp_err_t validate_connect_handler(httpd_req_t *req);

esp_err_t file_serve_handler(httpd_req_t *req);

httpd_handle_t start_webserver(void);

void check_wifi_task(void* pvParameters);

void send_ws_message(const char *message);

void message_queue_task(void *pvParameters);

void websocket_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

void websocket_task(void *pvParameters);
