#include <lwip/ip4_addr.h>

extern bool connected_to_WiFi;
extern char ESP_IP[16];

void send_arp_request(ip4_addr_t target_ip);

void scan_network(const char *subnet);

void print_arp_table();

void get_devices_task(void *pvParameters);
