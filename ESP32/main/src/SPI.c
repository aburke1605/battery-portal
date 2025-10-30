#include "SPI.h"

#include "config.h"
#include "global.h"

#include "driver/gpio.h"
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

static const char* TAG = "SPI";

static spi_device_handle_t lora_spi;

void spi_reset() {
    gpio_set_direction(PIN_NUM_RST, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_NUM_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(PIN_NUM_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
}

uint8_t spi_read_register(uint8_t reg) {
    uint8_t rx_data[2];
    uint8_t tx_data[2] = { reg & 0b01111111, 0x00 }; // MSB=0 for read

    spi_transaction_t t = {
        .length = 8 * 2,
        .tx_buffer = tx_data,
        .rx_buffer = rx_data,
    };
    spi_device_transmit(lora_spi, &t);
    return rx_data[1];
}

void spi_write_register(uint8_t reg, uint8_t value) {
    uint8_t tx_data[2] = { reg | 0b10000000, value }; // MSB=1 for write

    spi_transaction_t t = {
        .length = 8 * 2,
        .tx_buffer = tx_data,
    };
    spi_device_transmit(lora_spi, &t);
}

esp_err_t spi_init() {
    // SPI bus configuration
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);

    // SPI device configuration
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 8 * 1000 * 1000, // 8 MHz
        .mode = 0,
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 1,
    };
    spi_bus_add_device(SPI2_HOST, &devcfg, &lora_spi);

    spi_reset();

    // Read version
    uint8_t version = spi_read_register(REG_VERSION);
    ESP_LOGI(TAG, "SX127x version: 0x%02X", version);
    if (version != 0x12) {
        ESP_LOGE(TAG, "SX127x not found. LoRa not configured!");
        return ESP_FAIL;
    }

    // Enter LoRa + Sleep mode
    spi_write_register(REG_OP_MODE, MODE_SLEEP | MODE_LORA);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Enter standby
    spi_write_register(REG_OP_MODE, MODE_STDBY | MODE_LORA);
    vTaskDelay(pdMS_TO_TICKS(10));

    gpio_set_direction(PIN_NUM_DIO0, GPIO_MODE_INPUT);

    ESP_LOGI(TAG, "SX127x initialized");

    return ESP_OK;
}
