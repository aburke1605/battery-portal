#pragma once

#include "esp_err.h"
#include "esp_log.h"
#include "gpio_num.h"
#include "gpio_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*gpio_isr_t)(void *arg);

typedef struct {
  uint64_t pin_bit_mask;
  gpio_mode_t mode;
  gpio_pullup_t pull_up_en;
  gpio_pulldown_t pull_down_en;
  gpio_int_type_t intr_type;
} gpio_config_t;

static inline esp_err_t gpio_config(const gpio_config_t *pGPIOConfig) {
  ESP_LOGI("[esp_driver_gpio_stub]", "gpio_config called");
  return ESP_OK;
}

static inline esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level) {
  ESP_LOGI("[esp_driver_gpio_stub]", "gpio_set_level called");
  return ESP_OK;
}

static inline int gpio_get_level(gpio_num_t gpio_num) {
  ESP_LOGI("[esp_driver_gpio_stub]", "gpio_get_level called");
  return 1;
}

static inline esp_err_t gpio_set_direction(gpio_num_t gpio_num,
                                           gpio_mode_t mode) {
  ESP_LOGI("[esp_driver_gpio_stub]", "gpio_set_direction called");
  return ESP_OK;
}

static inline esp_err_t gpio_install_isr_service(int intr_alloc_flags) {
  ESP_LOGI("[esp_driver_gpio_stub]", "gpio_install_isr_service called");
  return ESP_OK;
}

static inline esp_err_t
gpio_isr_handler_add(gpio_num_t gpio_num, gpio_isr_t isr_handler, void *args) {
  ESP_LOGI("[esp_driver_gpio_stub]", "gpio_isr_handler_add called");
  return ESP_OK;
}

#ifdef __cplusplus
}
#endif
