#include <string.h>
#include <math.h>

#include "include/utils.h"

#include <esp_random.h>

#define EDUROAM_USERNAME CONFIG_EDUROAM_USERNAME
#define EDUROAM_PASSWORD CONFIG_EDUROAM_PASSWORD

void random_key(char *key) {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    size_t charset_size = sizeof(charset) - 1;

    for (size_t i = 0; i < KEY_LENGTH; i++) {
        key[i] = charset[esp_random() % charset_size];
    }
    key[KEY_LENGTH] = '\0';
}

void send_fake_post_request() {
    if (!connected_to_WiFi) {
        esp_http_client_config_t config = {
            .url = "http://192.168.4.1/validate_connect?id=eduroam",
            .method = HTTP_METHOD_POST,
        };

        esp_http_client_handle_t client = esp_http_client_init(&config);

        char post_data[128];
        snprintf(post_data, sizeof(post_data), "ssid=%s@liverpool.ac.uk&password=%s", EDUROAM_USERNAME, EDUROAM_PASSWORD);
        esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");
        esp_http_client_set_post_field(client, post_data, strlen(post_data));

        esp_http_client_perform(client);
        esp_http_client_cleanup(client);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

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

void url_encode(char *dest, const char *src, size_t dest_size) {
    const char *hex = "0123456789ABCDEF";
    size_t i, j = 0;

    for (i = 0; src[i] && j < dest_size - 1; i++) {
        if (('a' <= src[i] && src[i] <= 'z') ||
            ('A' <= src[i] && src[i] <= 'Z') ||
            ('0' <= src[i] && src[i] <= '9') ||
            (src[i] == '-' || src[i] == '_' || src[i] == '.' || src[i] == '~')) {
            dest[j++] = src[i];  // Keep safe characters unchanged
        } else {
            if (j + 3 < dest_size - 1) {  // Ensure enough space
                dest[j++] = '%';
                dest[j++] = hex[(src[i] >> 4) & 0xF];
                dest[j++] = hex[src[i] & 0xF];
            } else {
                break;  // Stop if out of space
            }
        }
    }
    dest[j] = '\0';  // Null-terminate
}

uint8_t get_block(uint8_t offset) {
    uint8_t block = (uint8_t)ceil((float)offset / 32.);
    if (block != 0) block -= 1;
    
    return block;
}
