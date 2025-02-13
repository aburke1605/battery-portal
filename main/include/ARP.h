#include <lwip/ip4_addr.h>

extern bool connected_to_WiFi;
extern char successful_ips[256][16];
extern char old_successful_ips[256][16];
extern uint8_t successful_ip_count;
extern uint8_t old_successful_ip_count;
extern char ESP_IP[16];

void send_arp_request(ip4_addr_t target_ip);

void print_arp_table();

void get_devices_task(void *pvParameters);
