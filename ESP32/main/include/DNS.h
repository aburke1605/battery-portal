#ifndef DNS_H
#define DNS_H

#include "lwip/sockets.h"

typedef struct {
    int sock;
    struct sockaddr_in source_addr;
    socklen_t socklen;
    uint8_t* buffer;
    size_t len;
    size_t capacity;
} dns_packet_t;

void handle_dns_request(dns_packet_t *packet);

void dns_server_freertos_task(void *arg);

#endif // DNS_H
