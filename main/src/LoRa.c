#include "../include/LoRa.h"

#include <string.h>

#define LORA_REG_FIFO 0x00
#define LORA_REG_OP_MODE 0x01
#define LORA_MODE_SLEEP 0x00
#define LORA_MODE_STDBY 0x01
#define LORA_MODE_TX 0x03
#define LORA_MODE_RX_CONTINUOUS 0x05

static esp_err_t lora_write_register_internal(lora_t *lora, uint8_t reg, uint8_t value);
static esp_err_t lora_read_register_internal(lora_t *lora, uint8_t reg, uint8_t *value);

esp_err_t lora_init(lora_t *lora, spi_host_device_t spi_host, gpio_num_t cs_pin, gpio_num_t reset_pin, gpio_num_t dio0_pin) {
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1 * 1000 * 1000, // 1 MHz
        .mode = 0,
        .spics_io_num = cs_pin,
        .queue_size = 1,
    };

    gpio_set_direction(reset_pin, GPIO_MODE_OUTPUT);
    gpio_set_direction(dio0_pin, GPIO_MODE_INPUT);

    esp_err_t ret = spi_bus_add_device(spi_host, &devcfg, &lora->spi);
    if (ret != ESP_OK) {
        return ret;
    }

    lora->cs_pin = cs_pin;
    lora->reset_pin = reset_pin;
    lora->dio0_pin = dio0_pin;

    return lora_reset(lora);
}

void lora_deinit(lora_t *lora) {
    spi_bus_remove_device(lora->spi);
}

esp_err_t lora_begin(lora_t *lora, long frequency) {
    esp_err_t ret = lora_write_register_internal(lora, LORA_REG_OP_MODE, LORA_MODE_SLEEP);
    if (ret != ESP_OK) return ret;

    // Set frequency (example for 868 MHz)
    uint64_t frf = ((uint64_t)frequency << 19) / 32000000;
    ret = lora_write_register_internal(lora, 0x06, (uint8_t)(frf >> 16));
    if (ret != ESP_OK) return ret;
    ret = lora_write_register_internal(lora, 0x07, (uint8_t)(frf >> 8));
    if (ret != ESP_OK) return ret;
    ret = lora_write_register_internal(lora, 0x08, (uint8_t)(frf));
    if (ret != ESP_OK) return ret;

    return lora_write_register_internal(lora, LORA_REG_OP_MODE, LORA_MODE_STDBY);
}

void lora_end(lora_t *lora) {
    lora_write_register_internal(lora, LORA_REG_OP_MODE, LORA_MODE_SLEEP);
}

esp_err_t lora_send_packet(lora_t *lora, const uint8_t *data, size_t length) {
    esp_err_t ret = lora_write_register_internal(lora, LORA_REG_OP_MODE, LORA_MODE_STDBY);
    if (ret != ESP_OK) return ret;

    // Write data to FIFO
    for (size_t i = 0; i < length; i++) {
        ret = lora_write_register_internal(lora, LORA_REG_FIFO, data[i]);
        if (ret != ESP_OK) return ret;
    }

    // Set mode to TX
    return lora_write_register_internal(lora, LORA_REG_OP_MODE, LORA_MODE_TX);
}

int lora_receive_packet(lora_t *lora, uint8_t *buffer, size_t length) {
    uint8_t value;
    esp_err_t ret = lora_read_register_internal(lora, LORA_REG_FIFO, &value);
    if (ret != ESP_OK) return -1;

    size_t i = 0;
    while (i < length) {
        buffer[i++] = value;
        ret = lora_read_register_internal(lora, LORA_REG_FIFO, &value);
        if (ret != ESP_OK) break;
    }

    return i;
}

void lora_set_tx_power(lora_t *lora, int level) {
    lora_write_register_internal(lora, 0x09, level);
}

void lora_set_spreading_factor(lora_t *lora, int sf) {
    lora_write_register_internal(lora, 0x1D, (sf << 4));
}

void lora_set_signal_bandwidth(lora_t *lora, long sbw) {
    uint8_t bw;
    if (sbw <= 7.8E3) bw = 0;
    else if (sbw <= 10.4E3) bw = 1;
    else if (sbw <= 15.6E3) bw = 2;
    else if (sbw <= 20.8E3) bw = 3;
    else if (sbw <= 31.25E3) bw = 4;
    else if (sbw <= 41.7E3) bw = 5;
    else if (sbw <= 62.5E3) bw = 6;
    else if (sbw <= 125E3) bw = 7;
    else if (sbw <= 250E3) bw = 8;
    else bw = 9;

    lora_write_register_internal(lora, 0x1D, (bw << 4));
}

void lora_set_coding_rate4(lora_t *lora, int denominator) {
    lora_write_register_internal(lora, 0x1D, (denominator << 1));
}

esp_err_t lora_write_register(lora_t *lora, uint8_t reg, uint8_t value) {
    return lora_write_register_internal(lora, reg, value);
}

esp_err_t lora_read_register(lora_t *lora, uint8_t reg, uint8_t *value) {
    return lora_read_register_internal(lora, reg, value);
}

esp_err_t lora_reset(lora_t *lora) {
    gpio_set_level(lora->reset_pin, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(lora->reset_pin, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    return ESP_OK;
}

esp_err_t lora_configure(lora_t *lora) {
    // Example configuration
    lora_set_tx_power(lora, 17);
    lora_set_spreading_factor(lora, 7);
    lora_set_signal_bandwidth(lora, 125E3);
    lora_set_coding_rate4(lora, 5);
    return ESP_OK;
}

static esp_err_t lora_write_register_internal(lora_t *lora, uint8_t reg, uint8_t value) {
    spi_transaction_t t = {
        .length = 16,
        .tx_data = {reg | 0x80, value},
        .flags = SPI_TRANS_USE_TXDATA,
    };
    return spi_device_transmit(lora->spi, &t);
}

static esp_err_t lora_read_register_internal(lora_t *lora, uint8_t reg, uint8_t *value) {
    spi_transaction_t t = {
        .length = 16,
        .tx_data = {reg & 0x7F, 0x00},
        .flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA,
    };
    esp_err_t ret = spi_device_transmit(lora->spi, &t);
    if (ret == ESP_OK) {
        *value = t.rx_data[1];
    }
    return ret;
}