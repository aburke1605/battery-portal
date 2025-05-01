#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define TAG "RFM95"

// SPI pins (adjust to your wiring)
#define PIN_NUM_MISO CONFIG_SPI_MISO_PIN
#define PIN_NUM_MOSI CONFIG_SPI_MOSI_PIN
#define PIN_NUM_CLK  CONFIG_SPI_SCK_PIN
#define PIN_NUM_CS   CONFIG_SPI_NSS_CS_PIN
#define PIN_NUM_RST  CONFIG_SPI_RESET_PIN
#define PIN_NUM_DIO0 CONFIG_SPI_DIO0_PIN

static spi_device_handle_t spi;

// Register addresses
#define REG_FIFO                0x00
#define REG_OP_MODE             0x01
#define REG_FRF_MSB             0x06
#define REG_FRF_MID             0x07
#define REG_FRF_LSB             0x08
#define REG_PA_CONFIG           0x09
#define REG_FIFO_ADDR_PTR       0x0D
#define REG_FIFO_TX_BASE_ADDR   0x0E
#define REG_PAYLOAD_LENGTH      0x22
#define REG_IRQ_FLAGS           0x12
#define REG_DIO_MAPPING_1       0x40

// SPI helper
void rfm95_write(uint8_t reg, uint8_t value) {
    uint8_t tx_data[2] = { (uint8_t)(reg | 0x80), value };
    spi_transaction_t t = {
        .length = 16,
        .tx_buffer = tx_data,
    };
    ESP_ERROR_CHECK(spi_device_transmit(spi, &t));
}

uint8_t rfm95_read(uint8_t reg) {
    uint8_t tx_data[2] = { reg & 0x7F, 0x00 };
    uint8_t rx_data[2];
    spi_transaction_t t = {
        .length = 16,
        .tx_buffer = tx_data,
        .rx_buffer = rx_data,
    };
    ESP_ERROR_CHECK(spi_device_transmit(spi, &t));
    return rx_data[1];
}

void rfm95_reset() {
    ESP_ERROR_CHECK(gpio_set_level(PIN_NUM_RST, 0));
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_ERROR_CHECK(gpio_set_level(PIN_NUM_RST, 1));
    vTaskDelay(pdMS_TO_TICKS(10));
}

void rfm95_configure() {
    rfm95_write(REG_OP_MODE, 0x80);  // LoRa sleep mode
    vTaskDelay(pdMS_TO_TICKS(10));

    // Set frequency to 868 MHz
    uint32_t frf = (uint32_t)((868000000.0 / 32000000.0) * (1 << 19));
    rfm95_write(REG_FRF_MSB, (frf >> 16) & 0xFF);
    rfm95_write(REG_FRF_MID, (frf >> 8) & 0xFF);
    rfm95_write(REG_FRF_LSB, frf & 0xFF);

    rfm95_write(REG_PA_CONFIG, 0x8F); // Max power
    rfm95_write(REG_FIFO_TX_BASE_ADDR, 0x00);
    rfm95_write(REG_FIFO_ADDR_PTR, 0x00);
    rfm95_write(REG_DIO_MAPPING_1, 0x40); // DIO0 = TxDone
    rfm95_write(REG_OP_MODE, 0x83); // LoRa standby mode
}

void rfm95_send_packet(const uint8_t* data, uint8_t len) {
    rfm95_write(REG_FIFO_ADDR_PTR, 0x00);
    for (int i = 0; i < len; i++) {
        rfm95_write(REG_FIFO, data[i]);
    }
    rfm95_write(REG_PAYLOAD_LENGTH, len);
    rfm95_write(REG_OP_MODE, 0x83); // Standby
    rfm95_write(REG_OP_MODE, 0x8B); // Transmit

    // Wait for TxDone
    printf("waiting...\n");
    while ((rfm95_read(REG_IRQ_FLAGS) & 0x08) == 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    rfm95_write(REG_IRQ_FLAGS, 0x08); // Clear TxDone
    ESP_LOGI(TAG, "Packet sent");
}
