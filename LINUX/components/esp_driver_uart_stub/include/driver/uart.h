#pragma once

#include "esp_err.h"
#include "esp_log.h"
#include "uart_types.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UART_PIN_NO_CHANGE      (-1)

typedef struct {
    int baud_rate;
    uart_word_length_t data_bits;
    uart_parity_t parity;
    uart_stop_bits_t stop_bits;
    uart_hw_flowcontrol_t flow_ctrl;
} uart_config_t;

static inline esp_err_t uart_driver_install(uart_port_t uart_num, int rx_buffer_size, int tx_buffer_size, int queue_size, QueueHandle_t* uart_queue, int intr_alloc_flags) {
    ESP_LOGI("[esp_driver_uart_stub]", "uart_driver_install called");
    return ESP_OK;
}

static inline esp_err_t uart_set_pin(uart_port_t uart_num, int tx_io_num, int rx_io_num, int rts_io_num, int cts_io_num) {
    ESP_LOGI("[esp_driver_uart_stub]", "uart_set_pin called");
    return ESP_OK;
}

static inline esp_err_t uart_param_config(uart_port_t uart_num, const uart_config_t *uart_config) {
    ESP_LOGI("[esp_driver_uart_stub]", "uart_param_config called");
    return ESP_OK;
}

static inline int uart_read_bytes(uart_port_t uart_num, void* buf, uint32_t length, TickType_t ticks_to_wait) {
    ESP_LOGI("[esp_driver_uart_stub]", "uart_read_bytes called");
    return 1;
}

#ifdef __cplusplus
}
#endif
