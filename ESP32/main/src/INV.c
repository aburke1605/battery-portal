#include "INV.h"

#include "TASK.h"
#include "config.h"
#include "global.h"

#include <string.h>

#include "driver/uart.h"
#include "esp_log.h"

static const char *TAG = "INV";

void uart_inv_init() {
  const uart_config_t uart_config = {.baud_rate = 9600,
                                     .data_bits = UART_DATA_8_BITS,
                                     .parity = UART_PARITY_DISABLE,
                                     .stop_bits = UART_STOP_BITS_1,
                                     .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};

  ESP_ERROR_CHECK(uart_param_config(INV_UART_NUM, &uart_config));
  ESP_ERROR_CHECK(uart_set_pin(INV_UART_NUM, INV_TX_GPIO, INV_RX_GPIO,
                               UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
  ESP_ERROR_CHECK(
      uart_driver_install(INV_UART_NUM, INV_BUFF_SIZE, 0, 0, NULL, 0));
}

void update_inv() {
  uint8_t msg[3] = {0x51, 0x30, 0x0D};
  uart_write_bytes(INV_UART_NUM, msg, sizeof(msg));

  uint8_t buff[INV_BUFF_SIZE];
  int len = uart_read_bytes(INV_UART_NUM, buff, INV_BUFF_SIZE - 1,
                            pdMS_TO_TICKS(1000));
  if (len > 0) {
    inverter_data.status = buff[3];
    inverter_data.output_voltage = buff[4] << 8 | buff[5];
    inverter_data.battery_voltage = buff[6] << 8 | buff[7];
    inverter_data.temperature = buff[8];
    inverter_data.output_power = buff[9] << 8 | buff[10];
  }

  return;
}
