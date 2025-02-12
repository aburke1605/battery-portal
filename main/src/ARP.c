#include <esp_log.h>
#include <esp_wifi.h>
#include <lwip/etharp.h>

#include "include/ping.h"

void send_arp_request(ip4_addr_t target_ip) {
    struct netif *netif = netif_default;  // Use default network interface
    if (!netif) {
        ESP_LOGE("ARP", "No default network interface!");
        return;
    }

    ESP_LOGI("ARP", "Sending ARP request for: " IPSTR, IP2STR(&target_ip));

    err_t res = etharp_request(netif, &target_ip);
    if (res != ERR_OK) {
        ESP_LOGE("ARP", "Failed to send ARP request: %d", res);
    }
}

void scan_network(const char *subnet) {
    ip4_addr_t target_ip;
    char ip_str[16];

    for (int i = 1; i < 255; i++) {  // Scan 192.168.x.1 - 192.168.x.254
        snprintf(ip_str, sizeof(ip_str), "%s.%d", subnet, i);
        if (ip4addr_aton(ip_str, &target_ip)) {
            send_arp_request(target_ip);
        }
        vTaskDelay(pdMS_TO_TICKS(10));  // Small delay to avoid flooding
    }
}

void print_arp_table() {
    struct eth_addr *eth_ret;
    struct netif *netif_ret;
    ip4_addr_t *ip_ret;

    ESP_LOGI("ARP", "Printing ARP Table:");
    for (size_t i = 0; i < ARP_TABLE_SIZE; i++) {
        if (etharp_get_entry(i, &ip_ret, &netif_ret, &eth_ret)) {
            printf("%s\n", ESP_IP);
            ESP_LOGI("ARP", "IP: " IPSTR ", MAC: %02X:%02X:%02X:%02X:%02X:%02X",
                IP2STR(ip_ret),
                eth_ret->addr[0], eth_ret->addr[1], eth_ret->addr[2],
                eth_ret->addr[3], eth_ret->addr[4], eth_ret->addr[5]);
        }
    }
}

void get_devices_task(void *pvParameters) {
    while (true) {
        if (connected_to_WiFi) {
            scan_network("192.168.137");
            vTaskDelay(pdMS_TO_TICKS(3000));  // Wait for ARP replies
            print_arp_table();
        }
        vTaskDelay(pdMS_TO_TICKS(5000));  // Print every 5 seconds
    }
}