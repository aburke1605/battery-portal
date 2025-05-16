#include "include/LoRa.h"

#include "include/BMS.h"
#include "include/global.h"
#include "include/utils.h"

#include <cJSON.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <esp_err.h>
#include <esp_log.h>

static spi_device_handle_t lora_spi;
extern QueueHandle_t lora_queue;

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
    uint8_t tx_data[2] = { reg & 0b01111111, 0x00 }; // MSB=0 for read

    spi_transaction_t t = {
        .length = 8 * 2,
        .tx_buffer = tx_data,
        .rx_buffer = rx_data,
    };
    spi_device_transmit(lora_spi, &t);
    return rx_data[1];
}

void lora_write_register(uint8_t reg, uint8_t value) {
    uint8_t tx_data[2] = { reg | 0b10000000, value }; // MSB=1 for write

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
    // Set carrier frequency
    uint64_t frf = ((uint64_t)(LORA_FREQ * 1E6) << 19) / 32000000;
    lora_write_register(REG_FRF_MSB, (uint8_t)(frf >> 16));
    lora_write_register(REG_FRF_MID, (uint8_t)(frf >> 8));
    lora_write_register(REG_FRF_LSB, (uint8_t)(frf >> 0));

    // Set LNA gain to maximum
    lora_write_register(REG_LNA, 0b00100011);  // LNA_MAX_GAIN | LNA_BOOST

    // Enable AGC (bit 2 of RegModemConfig3)
    lora_write_register(REG_MODEM_CONFIG_3,  // bits:
        (0b00001111 & 0)         << 4 |      //  7-4
        (0b00000001 & LORA_LDRO) << 3 |      //  3
        (0b00000001 & 1)         << 2 |      //  2    AGC on
        (0b00000011 & 0)                     //  1-0
    );  // LowDataRateOptimize off, AGC on

    // Configure modem parameters:
    //  bandwidth, coding rate, header
    lora_write_register(REG_MODEM_CONFIG_1,  // bits:
        (0b00001111 & LORA_BW)     << 4 |    //  7-4
        (0b00000111 & LORA_CR)     << 1 |    //  3-1
        (0b00000001 & LORA_HEADER)           //  0
    );
    //  spreading factor, cyclic redundancy check
    lora_write_register(REG_MODEM_CONFIG_2,     // bits:
        (0b00001111 & LORA_SF)          << 4 |  //  7-4
        (0b00000001 & LORA_Tx_CONT)     << 3 |  //  3
        (0b00000001 & LORA_Rx_PAYL_CRC) << 2 |  //  2
        (0b00000011 & 0)                        //  1-0
    );  // SF7, TxContinuousMode=0, CRC on

    // Preamble length (8 bytes = 0x0008)
    lora_write_register(REG_PREAMBLE_MSB, 0x00);
    lora_write_register(REG_PREAMBLE_LSB, 0x08);

    // Set output power to 13 dBm using PA_BOOST
    lora_write_register(REG_PA_CONFIG, 0b10001111);  // PA_BOOST, OutputPower=13 dBm

    ESP_LOGI(TAG, "SX127x configured to RadioHead defaults");
}

void lora_tx_task(void *pvParameters) {
    char individual_message[WS_MESSAGE_MAX_LEN];

    LoRa_message all_messages[5] = {0};
    // initiate
    for (int i=0; i<5; i++) strcpy(all_messages[i].id, "");

    char combined_message[3 + (sizeof(individual_message) + 2) * (5 + 1) - 2 + 3]; // +3 for "_S_", +2 for ", " between each message instance, +1 for this ESP32s own BMS data, -2 for strenuous ", ", +3 for "_E_"

    while (true) {
        if (xQueueReceive(lora_queue, individual_message, portMAX_DELAY) == pdPASS) {
            ESP_LOGI(TAG, "Received \"%s\" from queue", individual_message);
            cJSON *message_json = cJSON_Parse(individual_message);
            cJSON *id_obj = cJSON_GetObjectItem(message_json, "id");
            if (id_obj) {
                char *id = id_obj->valuestring;
                bool found = false;
                for (int i=0; i<5; i++) {
                    if (strcmp(all_messages[i].id, id) == 0) {
                        found = true;
                        strcpy(all_messages[i].message, individual_message);
                        break;
                    } else if (strcmp(all_messages[i].id, "") == 0) {
                        found = true;
                        strcpy(all_messages[i].id, id);
                        strcpy(all_messages[i].message, individual_message);
                        break;
                    }
                }

                if (!found) printf("error: no space!\n");
            }

            cJSON_Delete(message_json);
        }

        // now form the LoRa message out of non-empty messages and transmit
        size_t combined_capacity = sizeof(combined_message);
        combined_message[0] = '\0';
        strncat(combined_message, "_S_", combined_capacity - strlen("_S_") - 1);

        char *data_string = get_data();
        snprintf(individual_message, sizeof(individual_message), "{\"type\":\"data\",\"id\":\"%s\",\"content\":%s}", ESP_ID, data_string);
        strncat(combined_message, individual_message, combined_capacity - strlen(combined_message) - 1);

        for (int i=0; i<5; i++) {
            if (strcmp(all_messages[i].id, "") != 0) {
                strncat(combined_message, ", ", combined_capacity - strlen(combined_message) - 1);
                strncat(combined_message, all_messages[i].message, combined_capacity - strlen(combined_message) - 1);
            }
        }

        strncat(combined_message, "_E_", combined_capacity - strlen("_E_") - 1);

        if (strlen(combined_message) >= sizeof(combined_message) - 1) {
            ESP_LOGW(TAG, "combined_message may have been truncated!");
        }



        uint16_t full_len = strlen(combined_message);
        uint8_t chunk[LORA_MAX_PACKET_LEN + 1] = {0};  // room for payload + '\0'
        for (int offset = 0; offset < full_len; offset += LORA_MAX_PACKET_LEN) {
            int chunk_len = MIN(LORA_MAX_PACKET_LEN, full_len - offset);
            memcpy(chunk, combined_message + offset, chunk_len);
            chunk[chunk_len] = '\0';  // optional if treated as string


            // Set to standby
            lora_write_register(REG_OP_MODE, 0b10000001);  // LoRa + standby

            // Set DIO0 = TxDone
            lora_write_register(REG_DIO_MAPPING_1, 0b01000000); // bits 7-6 for DIO0

            // Set FIFO pointers
            lora_write_register(REG_FIFO_TX_BASE_ADDR, 0x00);  // FIFO TX base addr
            lora_write_register(REG_FIFO_ADDR_PTR, 0x00);  // FIFO addr ptr

            // Write payload to FIFO
            for (int i = 0; i < chunk_len; i++) {
                lora_write_register(REG_FIFO, chunk[i]);
            }

            lora_write_register(REG_PAYLOAD_LENGTH, chunk_len);  // Payload length

            // Clear all IRQ flags
            lora_write_register(REG_IRQ_FLAGS, 0b11111111);

            // Start transmission
            lora_write_register(REG_OP_MODE, 0b10000011);  // LoRa + TX mode

        ESP_LOGI(TAG, "Packet sent: \"%s\". Delaying for %d ms", combined_message, transmission_delay);
            // Wait for TX done (DIO0 goes high)
            while (gpio_get_level(PIN_NUM_DIO0) == 0) {
                vTaskDelay(pdMS_TO_TICKS(1));
            }

            // Clear IRQ
            lora_write_register(REG_IRQ_FLAGS, 0b00001000);  // Clear TxDone

            vTaskDelay(pdMS_TO_TICKS(50));  // brief delay between chunks
        }
        int transmission_delay = calculate_transmission_delay(LORA_SF, LORA_BW, 8, full_len, LORA_CR, LORA_HEADER, LORA_LDRO);

        vTaskDelay(pdMS_TO_TICKS(transmission_delay));
    }
}

void lora_rx_task(void *pvParameters) {
    uint8_t buffer[LORA_MAX_PACKET_LEN + 1];

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

                ESP_LOGI(TAG, "Received: \"%s\", RSSI: %d dBm", buffer, rssi_dbm);
            }

            // Clear IRQ flags
            lora_write_register(REG_IRQ_FLAGS, 0b11111111);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
