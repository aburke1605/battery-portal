#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_netif_types.h"

extern esp_netif_t *ap_netif;
extern bool is_root;
extern int num_connected_clients;
extern uint8_t ESP_ID;

#endif // GLOBAL_H
