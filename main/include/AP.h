#ifndef AP_H
#define AP_H

#include "include/config.h"

#include <esp_http_server.h>

struct rendered_page {
    char name[WS_MAX_HTML_PAGE_NAME_LENGTH];
    char content[WS_MAX_HTML_SIZE];
};

void wifi_init(void);

esp_err_t redirect_handler(httpd_req_t *req);

esp_err_t file_serve_handler(httpd_req_t *req);

httpd_handle_t start_webserver(void);

#endif // AP_H
