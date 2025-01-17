#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_http_server.h>

#define MAX_TASKS 10

extern char successful_ips[256][16];
extern uint8_t successful_ip_count;
extern char ESP_IP[16];

void register_task(TaskHandle_t task);

void unregister_task(TaskHandle_t task);

esp_err_t get_POST_data(httpd_req_t *req, char* content, size_t content_size);

void url_decode(char *dest, const char *src);

// void ping_target(const char *target_ip, int *current_ip);

typedef struct {
    uint32_t current_ip;
    esp_netif_ip_info_t ip_info;
} ping_context_t;
void ping_target(ping_context_t *ping_ctx);

void get_devices();

uint8_t get_block(uint8_t offset);
