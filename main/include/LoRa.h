#ifndef LORA_H
#define LORA_H

#include <string.h>

#include <esp_err.h>

#define LORA_SPI_HOST    SPI2_HOST   // or SPI3_HOST
#define PIN_NUM_MISO     19
#define PIN_NUM_MOSI     23
#define PIN_NUM_CLK      18
#define PIN_NUM_CS       5
#define PIN_NUM_RESET    26
#define PIN_NUM_DIO0     27

#define LORA_MAX_PACKET_LEN 255

// SX127x registers
#define REG_OP_MODE        0x01
#define REG_VERSION        0x42

// Modes
#define MODE_SLEEP         0b00000000
#define MODE_STDBY         0b00000001
#define MODE_LORA          0b10000000

void lora_reset();

uint8_t lora_read_register(uint8_t reg);

void lora_write_register(uint8_t reg, uint8_t value);

esp_err_t lora_init();

void lora_configure_defaults();

void lora_tx_task(void *pvParameters);

void lora_rx_task(void *pvParameters);

#endif // LORA_H
