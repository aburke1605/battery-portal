#pragma once

#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <esp_err.h>

typedef struct {
    spi_device_handle_t spi;
    gpio_num_t cs_pin;
    gpio_num_t reset_pin;
    gpio_num_t dio0_pin;
} lora_t;

esp_err_t lora_init(lora_t *lora, spi_host_device_t spi_host, gpio_num_t cs_pin, gpio_num_t reset_pin, gpio_num_t dio0_pin);
void lora_deinit(lora_t *lora);

esp_err_t lora_begin(lora_t *lora, long frequency);
void lora_end(lora_t *lora);

esp_err_t lora_send_packet(lora_t *lora, const uint8_t *data, size_t length);
int lora_receive_packet(lora_t *lora, uint8_t *buffer, size_t length);

void lora_set_tx_power(lora_t *lora, int level);
void lora_set_spreading_factor(lora_t *lora, int sf);
void lora_set_signal_bandwidth(lora_t *lora, long sbw);
void lora_set_coding_rate4(lora_t *lora, int denominator);

esp_err_t lora_write_register(lora_t *lora, uint8_t reg, uint8_t value);
esp_err_t lora_read_register(lora_t *lora, uint8_t reg, uint8_t *value);
esp_err_t lora_reset(lora_t *lora);
esp_err_t lora_configure(lora_t *lora);
