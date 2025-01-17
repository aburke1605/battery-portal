#include <string.h>
#include <math.h>

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


#define MAX_CONCURRENT_PINGS 5
#define PING_INTERVAL_MS 500 // optional delay between starting new pings
static SemaphoreHandle_t ping_semaphore;
static bool scanned_devices = false;
// callbacks...
void on_ping_success(esp_ping_handle_t hdl, void *args) {
    uint32_t elapsed_time;
    esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_time, sizeof(elapsed_time));

    ping_context_t *ctx = (ping_context_t *)args;

    uint32_t prev_ip = htonl(ntohl(ctx->current_ip) - 1);
    if (ctx->current_ip != ctx->ip_info.ip.addr && ctx->current_ip != ctx->ip_info.gw.addr) {
        // skip own IP and gateway IP
        snprintf(successful_ips[successful_ip_count++], sizeof(successful_ips[0]), IPSTR, IP2STR((ip4_addr_t *)&ctx->current_ip));
    }
}
void on_ping_timeout(esp_ping_handle_t hdl, void *args) {
    // uint8_t *current_ip = (uint8_t *)args;
}
void on_ping_end(esp_ping_handle_t hdl, void *args) {
    esp_ping_stop(hdl); // clean up the session
    esp_ping_delete_session(hdl); // properly delete the session
    xSemaphoreGive(ping_semaphore); // release the semaphore
}
// ...for use in this function
void ping_target(ping_context_t *ping_ctx) {
    ip_addr_t target_addr;
    target_addr.type = IPADDR_TYPE_V4;
    target_addr.u_addr.ip4.addr = htonl(ntohl(ping_ctx->current_ip));

    esp_ping_config_t config = ESP_PING_DEFAULT_CONFIG();
    config.target_addr = target_addr;
    config.count = 1; // one ping attempt

    esp_ping_callbacks_t callbacks = {
        .on_ping_success = on_ping_success,
        .on_ping_timeout = on_ping_timeout,
        .on_ping_end = on_ping_end,
        .cb_args = ping_ctx, // pass IP tracker to callback
    };

    esp_ping_handle_t ping;
    if (esp_ping_new_session(&config, &callbacks, &ping) == ESP_OK) {
        esp_ping_start(ping);
    } else {
        // ESP_LOGW("utils", "Failed to create ping session for IP: %s", target_ip);
        xSemaphoreGive(ping_semaphore); // release semaphore on failure
    }
    vTaskDelay(pdMS_TO_TICKS(PING_INTERVAL_MS)); // optional delay
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
    suspend_all_except_current(); // pause all other tasks

    // get Wi-Fi station gateway
    esp_netif_t *sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (sta_netif == NULL) return;

    // retrieve the IP information
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(sta_netif, &ip_info);

    uint32_t network_addr = ip_info.ip.addr & ip_info.netmask.addr; // network address
    uint32_t broadcast_addr = network_addr | ~ip_info.netmask.addr; // broadcast address

    successful_ip_count = 0; // reset successful IP list
    ping_semaphore = xSemaphoreCreateCounting(MAX_CONCURRENT_PINGS, MAX_CONCURRENT_PINGS);

    ping_context_t ping_ctx = {
        .current_ip = network_addr + htonl(1), // Start with the first usable IP
        .ip_info = ip_info
    };
    while (ping_ctx.current_ip < broadcast_addr) {
        // wait until we can initiate a new ping
        xSemaphoreTake(ping_semaphore, portMAX_DELAY);
        ping_target(&ping_ctx);
        ping_ctx.current_ip = htonl(ntohl(ping_ctx.current_ip) + 1);
    }

    // wait for all remaining pings to finish
    for (int i = 0; i < MAX_CONCURRENT_PINGS; i++) {
        xSemaphoreTake(ping_semaphore, portMAX_DELAY);
    }

    vSemaphoreDelete(ping_semaphore); // clean up the semaphore
    resume_all_tasks(); // resume all paused tasks

    // log the discovered devices
    ESP_LOGI("utils", "Discovered addresses:");
    for (size_t i = 0; i < successful_ip_count; i++) {
        ESP_LOGI("utils", "  - %s", successful_ips[i]);
    }

    scanned_devices = true;
}

uint8_t get_block(uint8_t offset) {
    uint8_t block = (uint8_t)ceil((float)offset / 32.);
    if (block != 0) block -= 1;
    
    return block;
}
