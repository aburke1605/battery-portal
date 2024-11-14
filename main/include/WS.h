#include <esp_log.h>
#include <esp_http_server.h>
#include <lwip/sockets.h>

#define CONFIG_MAX_CLIENTS 5

/*
 * Structure holding server handle
 * and internal socket fd in order
 * to use out of request send
 */
struct async_resp_arg {
    httpd_handle_t hd;
    int fd;
};

void ws_async_send(void *arg);

esp_err_t trigger_async_send(httpd_handle_t handle, httpd_req_t *req);

esp_err_t websocket_handler(httpd_req_t *req);

esp_err_t test_websocket_handler(httpd_req_t *req);

void websocket_send_task(void *arg);

// Configure URI for WebSocket connection
static const httpd_uri_t ws = {
        .uri        = "/ws", // URI to establish WebSocket connection
        .method     = HTTP_GET,
        .handler    = test_websocket_handler,
        .user_ctx   = NULL,
        .is_websocket = true // Mark this URI as WebSocket
};

httpd_handle_t start_webserver(void);