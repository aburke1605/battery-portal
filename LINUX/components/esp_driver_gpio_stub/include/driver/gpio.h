#pragma once

#include "esp_err.h"
#include "esp_log.h"
#include "gpio_num.h"
#include "gpio_types.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level) {
    ESP_LOGI("[esp_driver_gpio_stub]", "gpio_set_level called");
    return ESP_OK;
}

static inline int gpio_get_level(gpio_num_t gpio_num) {
    ESP_LOGI("[esp_driver_gpio_stub]", "gpio_get_level called");
    return 1;
}

static inline esp_err_t gpio_set_direction(gpio_num_t gpio_num, gpio_mode_t mode) {
    ESP_LOGI("[esp_driver_gpio_stub]", "gpio_set_direction called");
    return ESP_OK;
}

#ifdef __cplusplus
}
#endif
