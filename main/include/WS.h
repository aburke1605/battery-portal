#ifndef WS_H
#define WS_H

#include "include/config.h"

#include <cJSON.h>

#include <esp_err.h>
#include <esp_http_server.h>

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
} ws_payload;

void add_client(int fd, const char* tkn, bool browser);

void remove_client(int fd);

esp_err_t client_handler(httpd_req_t *req);

esp_err_t perform_request(cJSON *message, cJSON *response);

void send_message(const char *message);
// void send_message(const ws_payload *message);

void message_queue_task(void *pvParameters);

void websocket_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

void websocket_task(void *pvParameters);

#endif // WS_H
