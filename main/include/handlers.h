#include <esp_log.h>
#include <esp_http_server.h>

esp_err_t index_handler(httpd_req_t *req);

void parse_post_data(const char *data, char *username, char *password);

esp_err_t display_handler(httpd_req_t *req);

esp_err_t image_get_handler(httpd_req_t *req);
