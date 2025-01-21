#include <string.h>
#include <math.h>

#include "include/utils.h"

esp_err_t get_POST_data(httpd_req_t *req, char* content, size_t content_size) {
    int ret, content_len = req->content_len;

    // ensure the content fits in the buffer
    if (content_len >= content_size) {
        return ESP_ERR_INVALID_SIZE;
    }

    // read the POST data
    ret = httpd_req_recv(req, content, content_len);
    if (ret <= 0) {
        return ESP_FAIL;
    }
    content[ret] = '\0'; // null-terminate the string

    return ESP_OK;
}

void url_decode(char *dest, const char *src) {
    char *d = dest;
    const char *s = src;

    while (*s) {
        if (*s == '%') {
            if (*(s + 1) && *(s + 2)) {
                char hex[3] = { *(s + 1), *(s + 2), '\0' };
                *d++ = (char)strtol(hex, NULL, 16);
                s += 3;
            }
        } else if (*s == '+') {
            *d++ = ' ';
            s++;
        } else {
            *d++ = *s++;
        }
    }
    *d = '\0';
}

uint8_t get_block(uint8_t offset) {
    uint8_t block = (uint8_t)ceil((float)offset / 32.);
    if (block != 0) block -= 1;
    
    return block;
}
