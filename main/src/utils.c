#include <cJSON.h>

#include "include/utils.h"
#include "include/WS.h"

#include "include/global.h"

#include <math.h>
#include <esp_log.h>
#include <esp_http_client.h>
#include <esp_random.h>

char* esp_id_string() {
    const uint8_t n_bytes = 8;
    char* esp_id = malloc(n_bytes); // enough for "bms_255\0"
    if (esp_id == NULL) {
        ESP_LOGE("utils", "Couldn't allocate memory for esp_id string!");
        return NULL;
    }
    snprintf(esp_id, n_bytes, "bms_%03d", ESP_ID);
    esp_id[n_bytes - 1] = '\0';
    return esp_id;
}

void change_esp_id(char* name) {
    if (strncmp(name, "bms_", 4) != 0) {
        ESP_LOGE("utils", "New name not formatted correctly: \"%s\", should begin with \"bms_\"", name);
        return;
    }
    ESP_ID = atoi(&name[4]);
}

void send_fake_request() {
    if (!connected_to_WiFi) {
        cJSON *message = cJSON_CreateObject();
        cJSON_AddStringToObject(message, "type", "request");
        cJSON *content = cJSON_CreateObject();
        cJSON_AddStringToObject(content, "summary", "connect-wifi");
        cJSON *data = cJSON_CreateObject();
        char* esp_id = esp_id_string();
        cJSON_AddStringToObject(data, "esp_id", esp_id);
        free(esp_id);
        if (UTILS_EDUROAM) {
            cJSON_AddStringToObject(data, "username", UTILS_EDUROAM_USERNAME);
            cJSON_AddStringToObject(data, "password", UTILS_EDUROAM_PASSWORD);
            cJSON_AddBoolToObject(data, "eduroam", true);
        } else {
            cJSON_AddStringToObject(data, "username", UTILS_ROUTER_SSID);
            cJSON_AddStringToObject(data, "password", UTILS_ROUTER_PASSWORD);
            cJSON_AddBoolToObject(data, "eduroam", false);
        }
        cJSON_AddItemToObject(content, "data", data);
        cJSON_AddItemToObject(message, "content", content);

        cJSON *response = cJSON_CreateObject();
        perform_request(message, response);

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

typedef struct {
    char *buffer;
    int buffer_len;
} http_response_t;
esp_err_t fake_login_http_event_handler(esp_http_client_event_t *evt) {
    http_response_t *user_data = (http_response_t *)evt->user_data;

    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // int copy_len = MIN(evt->data_len, user_data->buffer_len - 1);
                int copy_len = evt->data_len <= user_data->buffer_len - 1 ? evt->data_len : user_data->buffer_len - 1;
                memcpy(user_data->buffer, evt->data, copy_len);
                user_data->buffer[copy_len] = 0;  // Null-terminate
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}
char *send_fake_login_post_request() {
    char response_buffer[39 + UTILS_AUTH_TOKEN_LENGTH] = {0};
    http_response_t response = {
        .buffer = response_buffer,
        .buffer_len = 39 + UTILS_AUTH_TOKEN_LENGTH,
    };

    esp_http_client_config_t config = {
        .url = "http://192.168.4.1/api/users/login",
        .event_handler = fake_login_http_event_handler,
        // .method = HTTP_METHOD_POST,
        .user_data = &response,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    char post_data[128];
    char encoded_email[32];
    url_encode(encoded_email, WS_USERNAME, sizeof(encoded_email));
    char encoded_password[32];
    url_encode(encoded_password, WS_PASSWORD, sizeof(encoded_password));
    snprintf(post_data, sizeof(post_data), "{\"email\":\"%s\",\"password\":\"%s\"}", encoded_email, encoded_password);
    post_data[strlen(post_data)] = '\0';

    esp_http_client_set_url(client, config.url);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);
    esp_http_client_cleanup(client);
    vTaskDelay(pdMS_TO_TICKS(5000));

    if (err == ESP_OK) {
        if (VERBOSE) ESP_LOGI("MESH", "HTTP POST Status = %d, Response = %s", esp_http_client_get_status_code(client), response.buffer);
        cJSON *response_object = cJSON_Parse(response.buffer);
        cJSON *auth_token = cJSON_GetObjectItem(response_object, "auth_token");

        return auth_token->valuestring;
    }

    ESP_LOGE("login", "HTTP POST request failed: %s", esp_err_to_name(err));
    return "NONE";
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

void convert_uint_to_n_bytes(uint input, uint8_t *output, size_t n_bytes, bool little_endian) {
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

void random_token(char *key) {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    size_t charset_size = sizeof(charset) - 1;

    for (size_t i = 0; i < UTILS_AUTH_TOKEN_LENGTH; i++) {
        key[i] = charset[esp_random() % charset_size];
    }
    key[UTILS_AUTH_TOKEN_LENGTH - 1] = '\0';
}

int round_to_dp(float var, int ndp) {
    char str[40];
    // format to 0 d.p. after multiplying by 10^{ndp}
    sprintf(str, "%.0f", var*pow(10,ndp));
    // insert back into var
    sscanf(str, "%f", &var);

    return (int)var;
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

int compare_mac(const uint8_t *mac1, const uint8_t *mac2) {
    for (int i = 0; i < 6; i++) {
        if (mac1[i] < mac2[i]) return -1;
        if (mac1[i] > mac2[i]) return 1;
    }
    return 0;
}

float calculate_symbol_length(uint8_t spreading_factor, uint8_t bandwidth) {
    float frequencies[] = {7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125.0, 250.0, 500.0};
    if (bandwidth < 0 || bandwidth >= sizeof(frequencies) / sizeof(frequencies[0])) {
        ESP_LOGW("LoRa", "Undefined bandwidth key! Returning to default...");
        bandwidth = 7;
    }

    float t_sym = pow(2, (float)spreading_factor) / (frequencies[bandwidth]);

    return t_sym;
}

int calculate_transmission_delay(uint8_t spreading_factor, uint8_t bandwidth, uint8_t n_preamble_symbols, uint16_t payload_length, uint8_t coding_rate, bool header, bool low_data_rate_optimisation) {
    float t_sym = calculate_symbol_length(spreading_factor, bandwidth);
    if (VERBOSE) ESP_LOGI("LoRa", "symbol time: %f ms", t_sym);

    float n_payload_symbols = ceil((8*payload_length - 4*spreading_factor + 28 + 16 - (header?20:0)) / (spreading_factor - (low_data_rate_optimisation?2:0)) / 4);
    n_payload_symbols *= (coding_rate + 4);
    if (n_payload_symbols < 0) {
        n_payload_symbols = 0;
        ESP_LOGI("LoRa", "n_payload_symbols is 0!");
    }
    if (VERBOSE) ESP_LOGI("LoRa", "N payload symbols: %f", n_payload_symbols);

    float t_air = (4.25 + (float)n_preamble_symbols + n_payload_symbols) * t_sym;
    if (VERBOSE) ESP_LOGI("LoRa", "time on air: %f ms", t_air);

    float n_payloads_per_hour = 36e3 / t_air;
    if (VERBOSE) ESP_LOGI("LoRa", "number of payloads per hour: %f", n_payloads_per_hour);

    float time_delay_seconds = pow(60, 2) / n_payloads_per_hour;
    if (VERBOSE) ESP_LOGI("LoRa", "delay between messages: %f s", time_delay_seconds);

    return (int)(time_delay_seconds * 1e3);
}
