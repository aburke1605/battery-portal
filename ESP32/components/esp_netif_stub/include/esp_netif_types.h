#pragma once

#include "esp_netif_ip_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  esp_ip4_addr_t ip;
  esp_ip4_addr_t netmask;
  esp_ip4_addr_t gw;
} esp_netif_ip_info_t;

struct esp_netif_obj;
typedef struct esp_netif_obj esp_netif_t;

#ifdef __cplusplus
}
#endif
