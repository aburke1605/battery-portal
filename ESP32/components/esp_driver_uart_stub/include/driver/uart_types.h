#pragma once

#include "esp_err.h"
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  UART_DATA_8_BITS = 0x3,
} uart_word_length_t;
typedef enum {
  UART_PARITY_DISABLE = 0x0,
} uart_parity_t;
typedef enum {
  UART_STOP_BITS_1 = 0x1,
} uart_stop_bits_t;
typedef enum {
  UART_HW_FLOWCTRL_DISABLE = 0x0,
} uart_hw_flowcontrol_t;

typedef enum {
  UART_NUM_1,
} uart_port_t;

#ifdef __cplusplus
}
#endif
