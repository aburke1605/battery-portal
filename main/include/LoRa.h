#ifndef LORA_H
#define LORA_H

#include "include/config.h"

#include <string.h>

#include <esp_err.h>

#define LORA_SPI_HOST    SPI2_HOST   // or SPI3_HOST

// pin wirings
#define PIN_NUM_MISO CONFIG_SPI_MISO_PIN
#define PIN_NUM_MOSI CONFIG_SPI_MOSI_PIN
#define PIN_NUM_CLK  CONFIG_SPI_SCK_PIN
#define PIN_NUM_CS   CONFIG_SPI_NSS_CS_PIN
#define PIN_NUM_RST  CONFIG_SPI_RST_PIN
#define PIN_NUM_DIO0 CONFIG_SPI_DIO0_PIN

// default config
#define LORA_FREQ        CONFIG_FREQ
#define LORA_SF          CONFIG_SF
#define LORA_BW          CONFIG_BW
#define LORA_CR          CONFIG_CR
#ifdef CONFIG_HEADER
    #define LORA_HEADER true
#else
    #define LORA_HEADER false
#endif
#ifdef CONFIG_LDRO
    #define LORA_LDRO true
#else
    #define LORA_LDRO false
#endif
#ifdef CONFIG_Tx_CONT
    #define LORA_Tx_CONT true
#else
    #define LORA_Tx_CONT false
#endif
#ifdef CONFIG_Rx_PAYL_CRC
    #define LORA_Rx_PAYL_CRC true
#else
    #define LORA_Rx_PAYL_CRC false
#endif

// register addresses
#define REG_FIFO                0x00
#define REG_OP_MODE             0x01
#define REG_FRF_MSB             0x06
#define REG_FRF_MID             0x07
#define REG_FRF_LSB             0x08
#define REG_PA_CONFIG           0x09
#define REG_LNA                 0x0C
#define REG_FIFO_ADDR_PTR       0x0D
#define REG_FIFO_TX_BASE_ADDR   0x0E
#define REG_FIFO_RX_BASE_ADDR   0x0F
#define REG_IRQ_FLAGS           0x12
#define REG_MODEM_CONFIG_1      0x1D
#define REG_MODEM_CONFIG_2      0x1E
#define REG_PREAMBLE_MSB        0x20
#define REG_PREAMBLE_LSB        0x21
#define REG_PAYLOAD_LENGTH      0x22
#define REG_MODEM_CONFIG_3      0x26
#define REG_DIO_MAPPING_1       0x40
#define REG_VERSION             0x42

#define LORA_QUEUE_SIZE 10
#define LORA_MAX_PACKET_LEN 255

// Modes
#define MODE_SLEEP         0b00000000
#define MODE_STDBY         0b00000001
#define MODE_LORA          0b10000000


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
