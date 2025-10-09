#pragma once

#include "esp_bit_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GPIO_MODE_DEF_INPUT           (BIT0)
#define GPIO_MODE_DEF_OUTPUT          (BIT1)
typedef enum {
    GPIO_MODE_INPUT = GPIO_MODE_DEF_INPUT,
    GPIO_MODE_OUTPUT = GPIO_MODE_DEF_OUTPUT,
} gpio_mode_t;

#ifdef __cplusplus
}
#endif
