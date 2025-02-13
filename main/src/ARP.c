#include <esp_log.h>
#include <esp_wifi.h>
#include <lwip/etharp.h>

#include "include/ARP.h"

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
                snprintf(successful_ips[successful_ip_count++], sizeof(successful_ips[0]), IPSTR, IP2STR(ip_ret));
        }
    }
}

void get_devices_task(void *pvParameters) {
    while (true) {
        if (connected_to_WiFi) {
            // get and set the IP address of the ESP32
            esp_netif_t *sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
            if (sta_netif == NULL) return;
            esp_netif_ip_info_t ip_info;
            esp_netif_get_ip_info(sta_netif, &ip_info);
            esp_ip4addr_ntoa(&ip_info.ip, ESP_IP, 16);


            uint32_t network_addr = ip_info.ip.addr & ip_info.netmask.addr; // network address
            uint32_t broadcast_addr = network_addr | ~ip_info.netmask.addr; // broadcast address

            // save successful IPs and reset for next iteration
            old_successful_ip_count = successful_ip_count;
            for (size_t i = 0; i < old_successful_ip_count; i++) strcpy(old_successful_ips[i], successful_ips[i]);
            successful_ip_count = 0;

            // scan the network
            uint32_t current_ip = network_addr + htonl(1); // start with the first usable IP
            while (current_ip < broadcast_addr) {
                ip4_addr_t target_ip;
                ip4_addr_set_u32(&target_ip, current_ip);
                send_arp_request(target_ip);
                current_ip = htonl(ntohl(current_ip) + 1);
                vTaskDelay(pdMS_TO_TICKS(10)); // small delay to avoid flooding
            }

            // wait for ARP replies
            vTaskDelay(pdMS_TO_TICKS(3000));

            print_arp_table();
        }
        vTaskDelay(pdMS_TO_TICKS(5000));  // Print every 5 seconds
    }
}