#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct esp_ip4_addr {
    uint32_t addr;
};
typedef struct esp_ip4_addr esp_ip4_addr_t;
// rather than using lwip
#define IP4_ADDR(ipaddr, a,b,c,d) ((ipaddr)->addr = ((a)<<24)|((b)<<16)|((c)<<8)|(d))

#ifdef __cplusplus
}
#endif
