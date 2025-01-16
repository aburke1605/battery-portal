#include <string.h>
#include <math.h>

#include <esp_http_client.h>
#include <ping/ping_sock.h>

#include "include/utils.h"

extern TaskHandle_t task_handles[MAX_TASKS];
extern size_t task_count;
void register_task(TaskHandle_t task) {
    if (task_count < MAX_TASKS && task != NULL) {
        task_handles[task_count++] = task;
    }
}
void unregister_task(TaskHandle_t task) {
    for (size_t i = 0; i < task_count; i++) {
        if (task_handles[i] == task) {
            task_handles[i] = NULL;
            break;
        }
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


#define MAX_CONCURRENT_PINGS 1 // Adjust based on your system capacity
#define PING_INTERVAL_MS 10   // Optional delay between starting new pings
static SemaphoreHandle_t ping_semaphore;
static uint8_t current_ip = 0; // Track the current IP being pinged
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
    esp_ping_delete_session(hdl);

    // Signal that a session has ended
    xSemaphoreGive(ping_semaphore);

    // Start the next ping
    uint8_t *next_ip = (uint8_t *)args;
    (*next_ip)++;
    if (*next_ip < 256) {
        char ip_buffer[16];
        snprintf(ip_buffer, sizeof(ip_buffer), "192.168.137.%d", *next_ip);
        // vTaskDelay(pdMS_TO_TICKS(PING_INTERVAL_MS)); // Optional delay
        ping_target(ip_buffer, next_ip); // Pass the updated IP
    }
}
// ...for use in this function
void ping_target(const char *target_ip, uint8_t *ip_tracker) {
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
        .cb_args = ip_tracker, // pass IP tracker to callback
    };

    esp_ping_handle_t ping;
    if (esp_ping_new_session(&config, &callbacks, &ping) == ESP_OK) {
        esp_ping_start(ping);
    } else {
        ESP_LOGE("utils", "Failed to create ping session for IP: %s", target_ip);

void suspend_all_except_current() {
    TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
    for (size_t i = 0; i < task_count; i++) {
        if (task_handles[i] != current_task && task_handles[i] != NULL) {
            vTaskSuspend(task_handles[i]);
        }
    }
}
void resume_all_tasks() {
    for (size_t i = 0; i < task_count; i++) {
        if (task_handles[i] != NULL) {
            vTaskResume(task_handles[i]);
        }
    }
}
void get_devices() {
    ping_semaphore = xSemaphoreCreateCounting(MAX_CONCURRENT_PINGS, MAX_CONCURRENT_PINGS);

    for (int i = 0; i < MAX_CONCURRENT_PINGS; i++) {
        xSemaphoreTake(ping_semaphore, portMAX_DELAY);
        char ip_buffer[16];
        snprintf(ip_buffer, sizeof(ip_buffer), "192.168.137.%d", current_ip);
        ping_target(ip_buffer, &current_ip); // start initial batch
    }
}

uint8_t get_block(uint8_t offset) {
    uint8_t block = (uint8_t)ceil((float)offset / 32.);
    if (block != 0) block -= 1;
    
    return block;
}
