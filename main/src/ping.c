#include <esp_log.h>

#include "include/ping.h"

static SemaphoreHandle_t ping_semaphore;
bool scanned_devices = false;

// callbacks...
void on_ping_success(esp_ping_handle_t hdl, void *args) {
    ping_context_t *ping_ctx = (ping_context_t *)args;
    if (ping_ctx->current_ip != ping_ctx->ip_info.ip.addr && ping_ctx->current_ip != ping_ctx->ip_info.gw.addr) {
        // skip own IP and gateway IP
        snprintf(successful_ips[successful_ip_count++], sizeof(successful_ips[0]), IPSTR, IP2STR((ip4_addr_t *)&ping_ctx->current_ip));
    }
}

void on_ping_timeout(esp_ping_handle_t hdl, void *args) {
    // optional: log no device found
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
    target_addr.u_addr.ip4.addr = ping_ctx->current_ip;

    esp_ping_config_t config = ESP_PING_DEFAULT_CONFIG();
    config.target_addr = target_addr;
    config.count = 1; // one ping attempt

    esp_ping_callbacks_t callbacks = {
        .on_ping_success = on_ping_success,
        .on_ping_end = on_ping_end,
        .cb_args = ping_ctx, // pass IP tracker to callback
    };

    esp_ping_handle_t ping;
    if (esp_ping_new_session(&config, &callbacks, &ping) == ESP_OK) {
        esp_ping_start(ping);
    } else {
        xSemaphoreGive(ping_semaphore); // release semaphore on failure
    }
    vTaskDelay(pdMS_TO_TICKS(PING_INTERVAL_MS)); // optional delay
}

void get_devices_task(void *pvParameters) {
    while (true) {
        if (connected_to_WiFi) {

            // get Wi-Fi station gateway
            esp_netif_t *sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
            if (sta_netif == NULL) return;

            // retrieve the IP information
            esp_netif_ip_info_t ip_info;
            esp_netif_get_ip_info(sta_netif, &ip_info);

            esp_ip4addr_ntoa(&ip_info.ip, ESP_IP, 16);

            uint32_t network_addr = ip_info.ip.addr & ip_info.netmask.addr; // network address
            uint32_t broadcast_addr = network_addr | ~ip_info.netmask.addr; // broadcast address

            old_successful_ip_count = successful_ip_count;
            for (size_t i = 0; i < old_successful_ip_count; i++) strcpy(old_successful_ips[i], successful_ips[i]);
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
            ping_semaphore = NULL;

            // log the discovered devices
            if (successful_ip_count > 0) ESP_LOGI("ping", "Discovered addresses:");
            for (size_t i = 0; i < successful_ip_count; i++) {
                ESP_LOGI("ping", "  - %s", successful_ips[i]);
            }

            scanned_devices = true;
        }
        else {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}