#include "DNS.h"

#include "TASK.h"
#include "config.h"
#include "global.h"

#include <errno.h>

#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
#include "esp_netif_types.h"
#include "freertos/FreeRTOS.h"
#include "lwip/ip4_addr.h"
#include "lwip/sockets.h"

static const char* TAG = "DNS";

void handle_dns_request(dns_packet_t *packet) {
    uint8_t *buffer = packet->buffer;

    // set DNS response flags
    buffer[2] = 0x81; // response flag and authoritative answer
    buffer[3] = 0x80; // recursion available

    buffer[6] = 0x00; // questions count (already in the request, copying as is)
    buffer[7] = 0x01; // answer count: set to 1

    // append answer record after the question
    int response_offset = packet->len;

    // make sure there is room for at least the 16 bytes of answer beneath
    if (packet->capacity < response_offset + 16) {
        ESP_LOGW(TAG, "Not enough buffer capacity for DNS answer");
        return;
    }

    // point to the answer section, matching the question section
    buffer[response_offset++] = 0xc0; // pointer to name offset
    buffer[response_offset++] = 0x0c;

    // type A record (IPv4 address)
    buffer[response_offset++] = 0x00;
    buffer[response_offset++] = 0x01;

    // class IN
    buffer[response_offset++] = 0x00;
    buffer[response_offset++] = 0x01;

    // TTL (time to live) - 1 hour
    buffer[response_offset++] = 0x00;
    buffer[response_offset++] = 0x00;
    buffer[response_offset++] = 0x0e;
    buffer[response_offset++] = 0x10;

    // data length (IPv4 address length)
    buffer[response_offset++] = 0x00;
    buffer[response_offset++] = 0x04;

    // add the AP IP address as the answer (4 bytes)
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip_info);
    buffer[response_offset++] = ip4_addr1(&ip_info.ip);
    buffer[response_offset++] = ip4_addr2(&ip_info.ip);
    buffer[response_offset++] = ip4_addr3(&ip_info.ip);
    buffer[response_offset++] = ip4_addr4(&ip_info.ip);

    if (VERBOSE) ESP_LOGI(TAG, "DNS request received, responding with AP IP: " IPSTR, IP2STR(&ip_info.ip));
    int sent = sendto(packet->sock, buffer, response_offset, 0, (struct sockaddr *)&packet->source_addr, packet->socklen);
    if (sent < 0) ESP_LOGE(TAG, "sendto failed: errno %d", errno);
}

void dns_server_freertos_task(void *arg) {
    int sock;
    struct sockaddr_in dest_addr, source_addr;
    socklen_t socklen = sizeof(source_addr);
    uint8_t buffer[512];

    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(DNS_PORT);

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

    while (true) {
        size_t len = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&source_addr, &socklen);
        size_t max_extra = 64; // probable safe headroom for DNS answer
        if (len > 0) {
            dns_packet_t *packet = malloc(sizeof(dns_packet_t));
            if (!packet) continue;
            packet->buffer = malloc(len + max_extra);
            if (!packet->buffer) { free(packet); continue; }

            packet->sock = sock;
            packet->source_addr = source_addr;
            packet->socklen = socklen;
            memcpy(packet->buffer, buffer, len);
            packet->len = len;
            packet->capacity = len + max_extra;

            job_t job = {
                .type = JOB_DNS_REQUEST,
                .data = packet,
                .size = sizeof(dns_packet_t),
            };

            if (xQueueSend(job_queue, &job, 0) != pdPASS) {
                ESP_LOGW(TAG, "Queue full, dropping job");
                free(job.data);
            }
        } else {
            ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
        }
    }
}
