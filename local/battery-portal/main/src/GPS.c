#include "include/GPS.h"
#include "include/config.h"

#include "driver/uart.h"

static const char* TAG = "GPS";

void uart_init() {
    const uart_config_t uart_config = {
        .baud_rate = 9600, // typical for NEO-6M
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    ESP_ERROR_CHECK(uart_param_config(GPS_UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(GPS_UART_NUM, GPS_GPIO_NUM_32, GPS_GPIO_NUM_33, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE)); // TX, RX
    ESP_ERROR_CHECK(uart_driver_install(GPS_UART_NUM, GPS_BUFF_SIZE, 0, 0, NULL, 0));
}
