#include <esp_log.h>
#include <esp_http_server.h>

esp_err_t get_POST_data(httpd_req_t *req, char* content, size_t content_size);

void url_decode(char *dest, const char *src);
