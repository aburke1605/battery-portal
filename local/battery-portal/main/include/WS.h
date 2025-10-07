#ifndef WS_H
#define WS_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_http_server.h"
#include "cJSON.h"

void add_client(int fd, const char* tkn, bool browser, uint8_t esp_id);

esp_err_t client_handler(httpd_req_t *req);

esp_err_t perform_request(cJSON *message, cJSON *response);

#endif // WS_H
