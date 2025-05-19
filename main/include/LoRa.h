#ifndef LORA_H
#define LORA_H

#include "include/config.h"

#include <string.h>

#include <esp_err.h>

typedef struct {
    char id[10];
    char message[WS_MESSAGE_MAX_LEN];
} LoRa_message;


void lora_reset();

uint8_t lora_read_register(uint8_t reg);

void lora_write_register(uint8_t reg, uint8_t value);

esp_err_t lora_init();

void lora_configure_defaults();

void lora_tx_task(void *pvParameters);

void lora_rx_task(void *pvParameters);

#endif // LORA_H
