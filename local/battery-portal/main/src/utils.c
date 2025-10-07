#include "include/utils.h"

#include "include/config.h"
#include "include/global.h"

#include <string.h>
#include <ctype.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_spiffs.h"
#include "esp_random.h"

void change_esp_id(char* name) {
    if (strncmp(name, "bms_", 4) != 0) {
        ESP_LOGE("utils", "New name not formatted correctly: \"%s\", should begin with \"bms_\"", name);
        return;
    }
    char* id_str = &name[4];
    // stop at first non-digit
    for (char* p = id_str; *p; p++) {
        if (!isdigit((unsigned char)*p)) {
            *p = '\0'; // force terminate
            break;
        }
    }
    ESP_ID = atoi(id_str);
}

void initialise_nvs() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    nvs_handle_t nvs;
    esp_err_t err = nvs_open("WIFI", NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        ESP_LOGE("NVS", "Couldn't open NVS namespace");
        return;
    }

    size_t len;
    err = nvs_get_str(nvs, "SSID", NULL, &len);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        // not yet initialised - this must be first boot since the most recent flash
        nvs_set_str(nvs, "SSID", WIFI_SSID);
        nvs_set_str(nvs, "PASSWORD", WIFI_PASSWORD);
        nvs_set_u8(nvs, "AUTO_CONNECT", WIFI_AUTO_CONNECT?1:0);
        nvs_commit(nvs);
    }
    nvs_close(nvs);
}

void initialise_spiffs() {
    esp_vfs_spiffs_conf_t config_static = {
        .base_path = "/static",
        .partition_label = "static",
        .max_files = 5,
        .format_if_mount_failed = true
    };
    esp_err_t ret = esp_vfs_spiffs_register(&config_static);

    if (ret != ESP_OK) {
        ESP_LOGE("utils", "Failed to initialise SPIFFS (%s)", esp_err_to_name(ret));
        return;
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

void convert_uint_to_n_bytes(unsigned int input, uint8_t *output, size_t n_bytes, bool little_endian) {
    for(size_t i=0; i<n_bytes; i++)
        output[i] = (input >> ((little_endian?n_bytes-1-i:i)*8)) & 0xFF;
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

void random_token(char *key) {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    size_t charset_size = sizeof(charset) - 1;

    for (size_t i = 0; i < UTILS_AUTH_TOKEN_LENGTH; i++) {
        key[i] = charset[esp_random() % charset_size];
    }
    key[UTILS_AUTH_TOKEN_LENGTH - 1] = '\0';
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
