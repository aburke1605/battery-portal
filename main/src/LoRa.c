#include "include/LoRa.h"

#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <esp_err.h>
#include <esp_log.h>

static spi_device_handle_t lora_spi;

static const char* TAG = "LoRa";

void lora_reset() {
    gpio_set_direction(PIN_NUM_RST, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_NUM_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(PIN_NUM_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
}

uint8_t lora_read_register(uint8_t reg) {
    uint8_t rx_data[2];
    uint8_t tx_data[2] = { reg & 0x7F, 0x00 }; // MSB=0 for read

    spi_transaction_t t = {
        .length = 8 * 2,
        .tx_buffer = tx_data,
        .rx_buffer = rx_data,
    };
    spi_device_transmit(lora_spi, &t);
    return rx_data[1];
}

void lora_write_register(uint8_t reg, uint8_t value) {
    uint8_t tx_data[2] = { reg | 0x80, value }; // MSB=1 for write

    spi_transaction_t t = {
        .length = 8 * 2,
        .tx_buffer = tx_data,
    };
    spi_device_transmit(lora_spi, &t);
}

esp_err_t lora_init() {
    // SPI bus configuration
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    spi_bus_initialize(LORA_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);

    // SPI device configuration
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 8 * 1000 * 1000, // 8 MHz
        .mode = 0,
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 1,
    };
    spi_bus_add_device(LORA_SPI_HOST, &devcfg, &lora_spi);

    lora_reset();

    // Read version
    uint8_t version = lora_read_register(REG_VERSION);
    ESP_LOGI(TAG, "SX127x version: 0x%02X", version);
    if (version != 0x12) {
        ESP_LOGE(TAG, "SX127x not found");
        return ESP_FAIL;
    }

    // Enter LoRa + Sleep mode
    lora_write_register(REG_OP_MODE, MODE_SLEEP | MODE_LORA);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Enter standby
    lora_write_register(REG_OP_MODE, MODE_STDBY | MODE_LORA);
    vTaskDelay(pdMS_TO_TICKS(10));

    ESP_LOGI(TAG, "SX127x initialized");
    return ESP_OK;
}

void lora_configure_defaults() {
    // Set carrier frequency to 868.0 MHz
    uint64_t frf = ((uint64_t)868000000 << 19) / 32000000;
    lora_write_register(REG_FRF_MSB, (uint8_t)(frf >> 16));
    lora_write_register(REG_FRF_MID, (uint8_t)(frf >> 8));
    lora_write_register(REG_FRF_LSB, (uint8_t)(frf >> 0));

    // Set LNA gain to maximum
    lora_write_register(0x0C, 0b00100011);  // LNA_MAX_GAIN | LNA_BOOST

    // Enable AGC (bit 2 of RegModemConfig3)
    lora_write_register(0x26, 0b00000100);  // LowDataRateOptimize off, AGC on

    // Configure modem parameters: BW = 125kHz, CR = 4/5, Explicit header
    lora_write_register(0x1D, 0b01110010);  // BW=7(125kHz), CR=1(4/5), ImplicitHeader=0

    // Spreading factor = 7, CRC on
    lora_write_register(0x1E, 0b01110100);  // SF7, TxContinuousMode=0, CRC on

    // Preamble length (8 bytes = 0x0008)
    lora_write_register(0x20, 0x00);
    lora_write_register(0x21, 0x08);

    // Set output power to 13 dBm using PA_BOOST
    lora_write_register(REG_PA_CONFIG, 0b10001111);  // PA_BOOST, OutputPower=13 dBm

    ESP_LOGI(TAG, "SX127x configured to RadioHead defaults");
}

void lora_tx_task(void *pvParameters) {
    const char *msg = "Hello!";
    uint8_t len = strlen(msg);

    while (true) {
        // Set to standby
        lora_write_register(REG_OP_MODE, 0b10000001);  // LoRa + standby

        // Set DIO0 = TxDone
        lora_write_register(REG_DIO_MAPPING_1, 0b01000000); // bits 7-6 for DIO0

        // Set FIFO pointers
        lora_write_register(REG_FIFO_TX_BASE_ADDR, 0x00);  // FIFO TX base addr
        lora_write_register(REG_FIFO_ADDR_PTR, 0x00);  // FIFO addr ptr

        // Write payload to FIFO
        for (int i = 0; i < len; i++) {
            lora_write_register(REG_FIFO, msg[i]);
        }

        lora_write_register(REG_PAYLOAD_LENGTH, len);  // Payload length

        // Clear all IRQ flags
        lora_write_register(REG_IRQ_FLAGS, 0b11111111);

        // Start transmission
        lora_write_register(REG_OP_MODE, 0b10000011);  // LoRa + TX mode

        // Wait for TX done (DIO0 goes high)
        while (gpio_get_level(PIN_NUM_DIO0) == 0) {
            vTaskDelay(pdMS_TO_TICKS(1));
        }

        // Clear IRQ
        lora_write_register(REG_IRQ_FLAGS, 0b00001000);  // Clear TxDone

        ESP_LOGI("TX", "Packet sent: %s", msg);
        vTaskDelay(pdMS_TO_TICKS(5000));  // Repeat every 5s
    }
}

void lora_rx_task(void *pvParameters) {
    uint8_t buffer[LORA_MAX_PACKET_LEN];

    // Configure FIFO base addr for RX
    lora_write_register(REG_FIFO_RX_BASE_ADDR, 0x00);  // FIFO RX base addr
    lora_write_register(REG_FIFO_ADDR_PTR, 0x00);  // FIFO addr ptr

    // Clear IRQ flags
    lora_write_register(REG_IRQ_FLAGS, 0b11111111);

    // Enter continuous RX mode
    lora_write_register(REG_OP_MODE, 0b10000101);  // LoRa + RX_CONT

    while (true) {
        // Wait for RX done (DIO0 goes high)
        if (gpio_get_level(PIN_NUM_DIO0) == 1) {
            uint8_t irq_flags = lora_read_register(REG_IRQ_FLAGS);
            if (irq_flags & 0x40) {  // RX_DONE
                uint8_t len = lora_read_register(0x13);  // RX bytes
                uint8_t fifo_rx_current = lora_read_register(0x10);
                lora_write_register(REG_FIFO_ADDR_PTR, fifo_rx_current);

                for (int i = 0; i < len; i++) {
                    buffer[i] = lora_read_register(0x00); // 0x00 = REG_FIFO ?
                }
                buffer[len] = '\0';  // Null-terminate

                uint8_t rssi_raw = lora_read_register(0x1A);
                int rssi_dbm = -157 + rssi_raw;

                ESP_LOGI("RX", "Received: %s, RSSI: %d dBm", buffer, rssi_dbm);
            }

            // Clear IRQ flags
            lora_write_register(REG_IRQ_FLAGS, 0b11111111);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
