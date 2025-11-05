#pragma once

#include "esp_err.h"
#include "esp_log.h"
#include "spi_types.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct spi_transaction_t spi_transaction_t;
struct spi_transaction_t {
  size_t length;
  union {
    const void *tx_buffer;
  };
  union {
    void *rx_buffer;
  };
};

typedef struct {
  uint8_t mode;
  int clock_speed_hz;
  int spics_io_num;
  int queue_size;
} spi_device_interface_config_t;

typedef struct spi_device_t *spi_device_handle_t;
static inline esp_err_t
spi_bus_add_device(spi_host_device_t host_id,
                   const spi_device_interface_config_t *dev_config,
                   spi_device_handle_t *handle) {
  ESP_LOGI("[esp_driver_spi_stub]", "spi_bus_add_device called");
  return ESP_OK;
}

static inline esp_err_t spi_device_transmit(spi_device_handle_t handle,
                                            spi_transaction_t *trans_desc) {
  ESP_LOGI("[esp_driver_spi_stub]", "spi_device_transmit called");
  return ESP_OK;
}

#ifdef __cplusplus
}
#endif
