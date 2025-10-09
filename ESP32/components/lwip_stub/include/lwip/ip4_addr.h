#pragma once

#include "def.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IPADDR_ANY          ((uint32_t)0x00000000UL)
#define IP4_ADDR(ipaddr, a,b,c,d)  (ipaddr)->addr = PP_HTONL(LWIP_MAKEU32(a,b,c,d))

#define ip4_addr_get_byte(ipaddr, idx) (((const uint8_t*)(&(ipaddr)->addr))[idx])
#define ip4_addr1(ipaddr) ip4_addr_get_byte(ipaddr, 0)
#define ip4_addr2(ipaddr) ip4_addr_get_byte(ipaddr, 1)
#define ip4_addr3(ipaddr) ip4_addr_get_byte(ipaddr, 2)
#define ip4_addr4(ipaddr) ip4_addr_get_byte(ipaddr, 3)

#ifdef __cplusplus
}
#endif
