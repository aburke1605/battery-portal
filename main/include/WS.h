#ifndef WS_H
#define WS_H

#include "include/config.h"

#include <cJSON.h>

#include <esp_err.h>
#include <esp_http_server.h>

struct rendered_page {
    char name[WS_MAX_HTML_PAGE_NAME_LENGTH];
    char content[WS_MAX_HTML_SIZE];
};

void add_client(int fd);

void remove_client(int fd);

esp_err_t perform_request(cJSON *message, cJSON *response);

esp_err_t validate_login_handler(httpd_req_t *req);

esp_err_t websocket_handler(httpd_req_t *req);

esp_err_t file_serve_handler(httpd_req_t *req);

httpd_handle_t start_webserver(void);

void send_ws_message(const char *message);

void message_queue_task(void *pvParameters);

void websocket_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

void websocket_task(void *pvParameters);

#endif // WS_H
