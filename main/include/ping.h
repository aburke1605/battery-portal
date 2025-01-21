#include <ping/ping_sock.h>
#include <esp_wifi.h>

#define MAX_CONCURRENT_PINGS 1
#define PING_INTERVAL_MS 400 // optional delay between starting new pings

extern bool connected_to_WiFi;
extern char successful_ips[256][16];
extern char old_successful_ips[256][16];
extern uint8_t successful_ip_count;
extern uint8_t old_successful_ip_count;
extern char ESP_IP[16];

typedef struct {
    uint32_t current_ip;
    esp_netif_ip_info_t ip_info;
} ping_context_t;

void on_ping_success(esp_ping_handle_t hdl, void *args);

void on_ping_timeout(esp_ping_handle_t hdl, void *args);

void on_ping_end(esp_ping_handle_t hdl, void *args);

void ping_target(ping_context_t *ping_ctx);

void get_devices_task(void *pvParameters);

