#include <string.h>
#include <math.h>

#include <esp_http_client.h>
#include <ping/ping_sock.h>

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


// callbacks...
void on_ping_success(esp_ping_handle_t hdl, void *args) {
    uint32_t elapsed_time;
    esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_time, sizeof(elapsed_time));
    ESP_LOGI("utils", "Ping success! Time: %ld ms", elapsed_time);
}
void on_ping_timeout(esp_ping_handle_t hdl, void *args) {
    ESP_LOGW("utils", "Ping timeout!");
}
void on_ping_end(esp_ping_handle_t hdl, void *args) {
    esp_ping_stop(hdl);
    ESP_LOGI("utils", "Ping session ended and cleaned up.");
}
// ...for use in this function
void ping_target(const char *target_ip) {
    ip_addr_t target_addr;
    inet_pton(AF_INET, target_ip, &target_addr.u_addr.ip4);
    target_addr.type = IPADDR_TYPE_V4;

    esp_ping_config_t config = ESP_PING_DEFAULT_CONFIG();
    config.target_addr = target_addr;
    config.count = 1; // one ping attempt

    esp_ping_callbacks_t callbacks = {
        .on_ping_success = on_ping_success,
        .on_ping_timeout = on_ping_timeout,
        .on_ping_end = on_ping_end,
        .cb_args = NULL, // optional argument to pass to callbacks
    };

    esp_ping_handle_t ping;
    esp_ping_new_session(&config, &callbacks, &ping);
    esp_ping_start(ping);
    esp_ping_delete_session(ping);
}
void get_devices() {
    char ip_buffer[16]; // xxx.xxx.xxx.xxx
    for (uint8_t ip_4=0; ip_4<256; ip_4++) {
        snprintf(ip_buffer, sizeof(ip_buffer), "192.168.137.%d", ip_4);
        printf("testing %d\n", ip_4);
        ping_target(ip_buffer);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

uint8_t get_block(uint8_t offset) {
    uint8_t block = (uint8_t)ceil((float)offset / 32.);
    if (block != 0) block -= 1;
    
    return block;
}
