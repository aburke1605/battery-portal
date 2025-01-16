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


#define MAX_CONCURRENT_PINGS 5 // Adjust based on your system capacity
#define PING_INTERVAL_MS 1000   // Optional delay between starting new pings
static SemaphoreHandle_t ping_semaphore;
static uint8_t current_ip = 0; // Track the current IP being pinged
extern char successful_ips[256][16]; // Array to store successful IPs
static uint8_t successful_ip_count = 0; // Counter for successful IPs
// callbacks...
void on_ping_success(esp_ping_handle_t hdl, void *args) {
    uint32_t elapsed_time;
    esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_time, sizeof(elapsed_time));

    int *current_ip = (int *)args;
    snprintf(successful_ips[successful_ip_count++], sizeof(successful_ips[0]), "192.168.137.%d", *current_ip);

    ESP_LOGI("ping", "Ping success! IP: 192.168.137.%d, Time: %ld ms", *current_ip, elapsed_time);
    // xSemaphoreGive(ping_semaphore); // Release semaphore after success
}
void on_ping_timeout(esp_ping_handle_t hdl, void *args) {
    int *current_ip = (int *)args;
    ESP_LOGW("ping", "Ping timeout! IP: 192.168.137.%d", *current_ip);
    // xSemaphoreGive(ping_semaphore); // Release semaphore on timeout
}
void on_ping_end(esp_ping_handle_t hdl, void *args) {
    ESP_LOGI("ping", "Ping session ended.");
    esp_ping_stop(hdl); // Clean up the session
    esp_ping_delete_session(hdl); // Properly delete the session
    xSemaphoreGive(ping_semaphore); // Release the semaphore
}
// ...for use in this function
void ping_target(const char *target_ip, int *current_ip) {
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
        ESP_LOGI("ping", "Ping session started for IP: %s", target_ip);
    } else {
        ESP_LOGW("ping", "Failed to create ping session for IP: %s", target_ip);
        xSemaphoreGive(ping_semaphore); // Release semaphore on failure
    }
}

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
    suspend_all_except_current(); // Pause all other tasks

    successful_ip_count = 0; // Reset successful IP list
    ping_semaphore = xSemaphoreCreateCounting(MAX_CONCURRENT_PINGS, MAX_CONCURRENT_PINGS);

    int current_ip = 0;
    while (current_ip < 256) {
        // Wait until we can initiate a new ping
        xSemaphoreTake(ping_semaphore, portMAX_DELAY);
        char ip_buffer[16];
        snprintf(ip_buffer, sizeof(ip_buffer), "192.168.137.%d", current_ip);
        vTaskDelay(pdMS_TO_TICKS(PING_INTERVAL_MS)); // Optional delay
        ping_target(ip_buffer, &current_ip);
        current_ip++;
    }

    // Wait for all remaining pings to finish
    for (int i = 0; i < MAX_CONCURRENT_PINGS; i++) {
        xSemaphoreTake(ping_semaphore, portMAX_DELAY);
    }

    vSemaphoreDelete(ping_semaphore); // Clean up the semaphore
    resume_all_tasks(); // Resume all paused tasks

    // Log the discovered devices
    ESP_LOGI("get_devices", "Discovered devices:");
    for (size_t i = 0; i < successful_ip_count; i++) {
        ESP_LOGI("get_devices", "  - %s", successful_ips[i]);
    }
}

uint8_t get_block(uint8_t offset) {
    uint8_t block = (uint8_t)ceil((float)offset / 32.);
    if (block != 0) block -= 1;
    
    return block;
}
