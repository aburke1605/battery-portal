#ifndef LORA_H
#define LORA_H

#include "include/config.h"

#include <string.h>

#include <esp_err.h>

typedef struct __attribute__((packed)) {
    uint8_t type;
    uint8_t esp_id;
    uint8_t Q;
    uint8_t H;
    uint8_t V;
    int8_t I;
    int16_t aT;
    int16_t iT;
    uint16_t BL;
    uint16_t BH;
    uint8_t CCT;
    uint8_t DCT;
    int8_t CITL;
    uint16_t CITH;
} radio_payload;

#define FRAME_END     0x7E  // marks beginning and end of message
#define FRAME_ESC     0x7D  // escape character
#define ESC_END       0x5E  // escaped 0x7E → 0x7D 0x5E
#define ESC_ESC       0x5D  // escaped 0x7D → 0x7D 0x5D

void lora_reset();

uint8_t lora_read_register(uint8_t reg);

void lora_write_register(uint8_t reg, uint8_t value);

esp_err_t lora_init();

void lora_configure_defaults();

radio_payload* convert_to_binary(char* message);

size_t encode_frame(const uint8_t* input, size_t input_len, uint8_t* output);

size_t decode_frame(const uint8_t* input, size_t input_len, uint8_t* output);

void convert_from_binary(uint8_t* decoded_message);

void lora_tx_task(void *pvParameters);

void lora_rx_task(void *pvParameters);

void lora_task(void *pvParameters);

#endif // LORA_H
