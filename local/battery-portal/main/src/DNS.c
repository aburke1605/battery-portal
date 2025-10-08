#include "include/DNS.h"

#include "include/config.h"

#include <errno.h>
#include "esp_log.h"
#include "lwip/sockets.h"
#include "lwip/ip4_addr.h"
#include "freertos/FreeRTOS.h"
#include "esp_netif_types.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"

static const char* TAG = "DNS";

// DNS handler (redirect all requests to our AP IP)
void dns_server_task(void *pvParameters) {
    struct sockaddr_in dest_addr;
    struct sockaddr_in source_addr;
    uint8_t buffer[512];
    int addr_family = AF_INET;
    int ip_protocol = IPPROTO_IP;

    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(DNS_PORT);

    // Create UDP socket
    int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
    }
    if (bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        close(sock);
        vTaskDelete(NULL);
    }

    ESP_LOGI(TAG, "DNS server started on port %d", DNS_PORT);

    // Get AP IP address
    esp_netif_ip_info_t ip_info = {0};
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if (netif == NULL || esp_netif_get_ip_info(netif, &ip_info) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get AP IP address");
        vTaskDelete(NULL);
    }

    while (true) {
        socklen_t socklen = sizeof(source_addr);
        int len = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&source_addr, &socklen);

        if (len > 0) {
            if (VERBOSE) ESP_LOGI(TAG, "DNS request received, responding with AP IP: " IPSTR, IP2STR(&ip_info.ip));

            // Set the DNS response flags
            buffer[2] = 0x81; // Response flag and authoritative answer
            buffer[3] = 0x80; // Recursion available

            // Questions count (already in the request, copying as is)
            // Answer count: Set to 1
            buffer[6] = 0x00;
            buffer[7] = 0x01;

            // Append answer record after the question
            int response_offset = len;

            // Point to the answer section, matching the question section
            buffer[response_offset++] = 0xc0; // Pointer to name offset
            buffer[response_offset++] = 0x0c;

            // Type A record (IPv4 address)
            buffer[response_offset++] = 0x00;
            buffer[response_offset++] = 0x01;

            // Class IN
            buffer[response_offset++] = 0x00;
            buffer[response_offset++] = 0x01;

            // TTL (Time to live) - 1 hour
            buffer[response_offset++] = 0x00;
            buffer[response_offset++] = 0x00;
            buffer[response_offset++] = 0x0e;
            buffer[response_offset++] = 0x10;

            // Data length (IPv4 address length)
            buffer[response_offset++] = 0x00;
            buffer[response_offset++] = 0x04;

            // Add the AP IP address as the answer (4 bytes)
            buffer[response_offset++] = ip4_addr1(&ip_info.ip);
            buffer[response_offset++] = ip4_addr2(&ip_info.ip);
            buffer[response_offset++] = ip4_addr3(&ip_info.ip);
            buffer[response_offset++] = ip4_addr4(&ip_info.ip);

            // Send DNS response
            sendto(sock, buffer, response_offset, 0, (struct sockaddr *)&source_addr, socklen);
        } else {
            ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
        }
    }

    // Cleanup if the loop exits
    close(sock);
    vTaskDelete(NULL);
}