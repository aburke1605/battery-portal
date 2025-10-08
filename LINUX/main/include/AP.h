#ifndef AP_H
#define AP_H

#include "config.h"

#include "esp_http_server.h"
#include "esp_wifi_types_generic.h"

struct rendered_page {
    char name[WS_MAX_HTML_PAGE_NAME_LENGTH];
    char content[WS_MAX_HTML_SIZE];
};

wifi_ap_record_t *wifi_scan(void);

void ap_n_clients_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

void wifi_init(void);

esp_err_t redirect_handler(httpd_req_t *req);

esp_err_t file_serve_handler(httpd_req_t *req);

esp_err_t login_handler(httpd_req_t *req);

esp_err_t num_clients_handler(httpd_req_t *req);

esp_err_t restart_handler(httpd_req_t *req);

httpd_handle_t start_webserver(void);

#endif // AP_H
