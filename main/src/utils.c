#include <string.h>
#include <math.h>

#include "include/config.h"
#include "include/utils.h"

#include <esp_random.h>

void random_key(char *key) {
    //
    // not used currently
    //

    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    size_t charset_size = sizeof(charset) - 1;

    for (size_t i = 0; i < UTILS_KEY_LENGTH; i++) {
        key[i] = charset[esp_random() % charset_size];
    }
    key[UTILS_KEY_LENGTH] = '\0';
}

void send_fake_post_request() {
    if (!connected_to_WiFi) {
        char url[64];
        snprintf(url, sizeof(url), "http://%s/validate_connect%s", ESP_subnet_IP, UTILS_EDUROAM ? "?eduroam=true" : "");
        esp_http_client_config_t config = {
            .url = url,
            .method = HTTP_METHOD_POST,
        };

        esp_http_client_handle_t client = esp_http_client_init(&config);

        char post_data[128];
        if (UTILS_EDUROAM) {
            snprintf(post_data, sizeof(post_data), "ssid=%s@liverpool.ac.uk&password=%s", UTILS_EDUROAM_USERNAME, UTILS_EDUROAM_PASSWORD);
        } else {
            strcpy(post_data, "ssid=Aodhan's%20Laptop&password=32mF%2B669");
            post_data[strlen(post_data)] = '\0';
        }
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

char* read_file(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    size_t file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* buffer = (char*)malloc(file_size + 1);
    if (!buffer){
        fclose(f);
        return NULL;
    }

    size_t bytes_read = fread(buffer, 1, file_size, f);
    if (bytes_read != file_size) {
        free(buffer);
        fclose(f);
        return NULL;
    }
    buffer[bytes_read] = '\0';

    fclose(f);
    return buffer;
}

char* replace_placeholder(const char *html, const char *const placeholders[], const char*const substitutes[], size_t num_replacements) {
    if (!html || !placeholders || !substitutes) return NULL;

    size_t new_len = strlen(html);
    size_t i, count;

    for (i = 0; i < num_replacements; i++) {
        // count occurrences of placeholder
        count = 0;
        const char* tmp = html;
        while ((tmp = strstr(tmp, placeholders[i]))) {
            count++;
            tmp += strlen(placeholders[i]);
        }

        new_len += count * (strlen(substitutes[i]) - strlen(placeholders[i]));
    }

    // allocate memory for new page
    char *result = malloc(new_len + 1);
    if (!result) return NULL;

    // remove occurrences
    char *dest = result;
    while (*html) {
        bool replaced = false;
        for (i = 0; i< num_replacements; i++) {
            if (strncmp(html, placeholders[i], strlen(placeholders[i])) == 0) {
                memcpy(dest, substitutes[i], strlen(substitutes[i]));
                html += strlen(placeholders[i]);
                dest += strlen(substitutes[i]);

                replaced = true;
                break;
            }
        }

        if (!replaced) {
            *dest++ = *html++;
        }
    }
    *dest = '\0';

    return result;
}

uint8_t get_block(uint8_t offset) {
    uint8_t block = (uint8_t)ceil((float)offset / 32.);
    if (block != 0) block -= 1;
    
    return block;
}

void check_bytes(TaskParams *params) {
    int remaining_task_stack = (int)uxTaskGetStackHighWaterMark(NULL);
    if (remaining_task_stack < (int)(0.3 * params->stack_size)) {
        ESP_LOGW(params->task_name, "Less than 30%% of assigned memory (%d bytes) being used - consider assigning less!\n", params->stack_size);
    } else if (remaining_task_stack > (int)(0.9 * params->stack_size)) {
        ESP_LOGW(params->task_name, "More than 90%% of assigned memory (%d bytes) being used - consider assigning more!\n", params->stack_size);
    }
}
