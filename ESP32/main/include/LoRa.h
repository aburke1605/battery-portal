#ifndef LORA_H
#define LORA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "cJSON.h"

enum packet_type {
    DATA,
    QUERY,
    REQUEST,
    RESPONSE,
};

// `type` must always be first byte in each type of radio packet
typedef struct __attribute__((packed)) {
    float t;
    int d;
    float lat;
    float lon;
    uint8_t type;
    uint8_t esp_id;
    uint8_t Q;
    uint8_t H;
    uint8_t V;
    uint16_t V1;
    uint16_t V2;
    uint16_t V3;
    uint16_t V4;
    int8_t I;
    int16_t I1;
    int16_t I2;
    int16_t I3;
    int16_t I4;
    int16_t aT;
    int16_t cT;
    int16_t T1;
    int16_t T2;
    int16_t T3;
    int16_t T4;
    int16_t OTC;
    bool wifi;
} radio_data_packet;

typedef struct __attribute__((packed)) {
    uint8_t type;
    uint8_t esp_id;
    int8_t query; // 1 for "are you still there?"
} radio_query_packet;

enum request_type {
    NO_REQUEST,
    CHANGE_SETTINGS,
    CONNECT_WIFI,
    RESET_BMS,
    UNSEAL_BMS,
};
typedef struct __attribute__((packed)) {
    uint8_t type;
    uint8_t esp_id;
    int8_t request;
    uint8_t new_esp_id;
    int16_t OTC;
    uint8_t ssid[16];
    uint8_t password[16];
    bool auto_connect;
    bool success;
} radio_request_packet;

#define FRAME_END     0x7E  // marks beginning and end of message
#define FRAME_ESC     0x7D  // escape character
#define ESC_END       0x5E  // escaped 0x7E → 0x7D 0x5E
#define ESC_ESC       0x5D  // escaped 0x7D → 0x7D 0x5D

void lora_init();

size_t json_to_binary(uint8_t* binary_message, cJSON* json_array);

size_t encode_frame(const uint8_t* input, size_t input_len, uint8_t* output);

size_t decode_frame(const uint8_t* input, size_t input_len, uint8_t* output);

void binary_to_json(uint8_t* binary_message, cJSON* json_array);

void receive();

void execute_transmission(uint8_t* message, size_t n_bytes);

void transmit();

void lora_task(void *pvParameters);

void start_receive_interrupt_task();

void start_transmit_timed_task();

#endif // LORA_H
