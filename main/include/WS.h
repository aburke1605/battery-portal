#include <esp_log.h>
#include <esp_http_server.h>
#include <lwip/sockets.h>

extern httpd_handle_t server;

#define CONFIG_MAX_CLIENTS 5
static int client_sockets[CONFIG_MAX_CLIENTS];

void add_client(int fd);

void remove_client(int fd);

esp_err_t login_handler(httpd_req_t *req);

esp_err_t display_handler(httpd_req_t *req);

esp_err_t websocket_handler(httpd_req_t *req);

esp_err_t image_handler(httpd_req_t *req);

httpd_handle_t start_webserver(void);

void websocket_broadcast_task(void *pvParameters);
