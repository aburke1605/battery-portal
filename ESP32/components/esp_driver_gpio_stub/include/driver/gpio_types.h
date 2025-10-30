#pragma once

#include "esp_bit_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  GPIO_INTR_POSEDGE = 1,
} gpio_int_type_t;

#define GPIO_MODE_DEF_INPUT (BIT0)
#define GPIO_MODE_DEF_OUTPUT (BIT1)
typedef enum {
  GPIO_MODE_INPUT = GPIO_MODE_DEF_INPUT,
  GPIO_MODE_OUTPUT = GPIO_MODE_DEF_OUTPUT,
} gpio_mode_t;

typedef enum {
  GPIO_PULLUP_DISABLE = 0x0,
} gpio_pullup_t;

typedef enum {
  GPIO_PULLDOWN_DISABLE = 0x0,
} gpio_pulldown_t;

#ifdef __cplusplus
}
#endif
