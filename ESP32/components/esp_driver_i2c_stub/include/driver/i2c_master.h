#pragma once

#include "driver/gpio_num.h"
#include "esp_err.h"
#include "esp_log.h"
#include "i2c_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  SOC_MOD_CLK_XTAL,
} soc_module_clk_t;
typedef enum {
  I2C_CLK_SRC_DEFAULT = SOC_MOD_CLK_XTAL,
} soc_periph_i2c_clk_src_t;
typedef soc_periph_i2c_clk_src_t i2c_clock_source_t;
typedef struct {
  i2c_port_num_t i2c_port;
  gpio_num_t sda_io_num;
  gpio_num_t scl_io_num;
  union {
    i2c_clock_source_t clk_source;
  };
  uint8_t glitch_ignore_cnt;
  struct {
    uint32_t enable_internal_pullup : 1;
  } flags;
} i2c_master_bus_config_t;

typedef struct {
  uint16_t device_address;
  uint32_t scl_speed_hz;
} i2c_device_config_t;

static inline esp_err_t
i2c_new_master_bus(const i2c_master_bus_config_t *bus_config,
                   i2c_master_bus_handle_t *ret_bus_handle) {
  ESP_LOGI("[esp_driver_i2c_stub]", "i2c_new_master_bus called");
  return ESP_OK;
}

static inline esp_err_t
i2c_master_bus_add_device(i2c_master_bus_handle_t bus_handle,
                          const i2c_device_config_t *dev_config,
                          i2c_master_dev_handle_t *ret_handle) {
  ESP_LOGI("[esp_driver_i2c_stub]", "i2c_master_bus_add_device called");
  return ESP_OK;
}

static inline esp_err_t i2c_master_transmit(i2c_master_dev_handle_t i2c_dev,
                                            const uint8_t *write_buffer,
                                            size_t write_size,
                                            int xfer_timeout_ms) {
  ESP_LOGI("[esp_driver_i2c_stub]", "i2c_master_transmit called");
  return ESP_OK;
}

static inline esp_err_t i2c_master_receive(i2c_master_dev_handle_t i2c_dev,
                                           uint8_t *read_buffer,
                                           size_t read_size,
                                           int xfer_timeout_ms) {
  ESP_LOGI("[esp_driver_i2c_stub]", "i2c_master_receive called");
  return ESP_OK;
}

static inline esp_err_t i2c_master_probe(i2c_master_bus_handle_t bus_handle,
                                         uint16_t address,
                                         int xfer_timeout_ms) {
  ESP_LOGI("[esp_driver_i2c_stub]", "i2c_master_probe called");
  return ESP_OK;
}

#ifdef __cplusplus
}
#endif
