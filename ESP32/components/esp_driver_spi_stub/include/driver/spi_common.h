#pragma once

#include "esp_err.h"
#include "esp_log.h"
#include "spi_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  SPI_DMA_CH_AUTO = 3,
} spi_common_dma_t;
#if __cplusplus
typedef int spi_dma_chan_t;
#else
typedef spi_common_dma_t spi_dma_chan_t;
#endif

typedef struct {
  union {
    int mosi_io_num;
  };
  union {
    int miso_io_num;
  };
  int sclk_io_num;
  union {
    int quadwp_io_num;
  };
  union {
    int quadhd_io_num;
  };
} spi_bus_config_t;

static inline esp_err_t spi_bus_initialize(spi_host_device_t host_id,
                                           const spi_bus_config_t *bus_config,
                                           spi_dma_chan_t dma_chan) {
  ESP_LOGI("[esp_driver_spi_stub]", "spi_bus_initialize called");
  return ESP_OK;
}

#ifdef __cplusplus
}
#endif
