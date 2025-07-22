#include "include/LoRa.h"

#include "include/BMS.h"
#include "include/global.h"
#include "include/WS.h"
#include "include/utils.h"

#include <cJSON.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_timer.h>

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
        esp_restart();
        return ESP_FAIL;
    }

    // Enter LoRa + Sleep mode
    lora_write_register(REG_OP_MODE, MODE_SLEEP | MODE_LORA);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Enter standby
    lora_write_register(REG_OP_MODE, MODE_STDBY | MODE_LORA);
    vTaskDelay(pdMS_TO_TICKS(10));

    ESP_LOGI(TAG, "SX127x initialized");

    lora_configure_defaults();
    gpio_set_direction(PIN_NUM_DIO0, GPIO_MODE_INPUT);

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
    lora_write_register(REG_MODEM_CONFIG_3,                                                                    // bits:
        (0b00001111 & 0)                                                                               << 4 |  //  7-4
        (0b00000001 & (LORA_LDRO | (calculate_symbol_length(LORA_SF, LORA_BW) > 16.0 ? true : false))) << 3 |  //  3
        (0b00000001 & 1)                                                                               << 2 |  //  2    AGC on
        (0b00000011 & 0)                                                                                       //  1-0
    );

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
    lora_write_register(REG_PA_CONFIG,           // bits:
        (0b00000001 & 1)                 << 7 |  // 7   PA BOOST
        (0b00000111 & 0x04)              << 4 |  // 6-4
        (0b00001111 & LORA_OUTPUT_POWER)
    );

    if (LORA_POWER_BOOST) lora_write_register(REG_PA_DAC, 0x87);
    else lora_write_register(REG_PA_DAC, 0x84);

    ESP_LOGI(TAG, "SX127x configured to RadioHead defaults");
}

radio_payload* convert_to_binary(char* message) {
    radio_payload* payload = malloc(sizeof(radio_payload));
    if (payload == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory\n");
        return NULL;
    }
    payload->type = 1;

    cJSON *json = cJSON_Parse(message);
    if (!json) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return NULL;
    }

    cJSON *id = cJSON_GetObjectItem(json, "id");
    if (!id) {
        ESP_LOGE(TAG, "Failed to parse JSON: no id");
        return NULL;
    }
    if (strcmp(id->valuestring, "unknown") == 0)
        payload->esp_id = 0;
    else {
        char esp_id_number[3];
        strcpy(esp_id_number, &id->valuestring[4]);
        payload->esp_id = atoi(esp_id_number);
    }

    cJSON *content = cJSON_GetObjectItem(json, "content");
    if (!content) {
        ESP_LOGE(TAG, "Failed to parse JSON: no content");
        return NULL;
    }
    cJSON *obj;

    obj = cJSON_GetObjectItem(content, "Q");
    if (obj) payload->Q = (uint8_t)obj->valueint;
    obj = NULL;

    obj = cJSON_GetObjectItem(content, "H");
    if (obj) payload->H = (uint8_t)obj->valueint;
    obj = NULL;

    obj = cJSON_GetObjectItem(content, "V");
    if (obj) payload->V = (uint8_t)obj->valueint;
    obj = NULL;

    obj = cJSON_GetObjectItem(content, "V1");
    if (obj) payload->V1 = (uint16_t)obj->valueint;
    obj = NULL;

    obj = cJSON_GetObjectItem(content, "V2");
    if (obj) payload->V2 = (uint16_t)obj->valueint;
    obj = NULL;

    obj = cJSON_GetObjectItem(content, "V3");
    if (obj) payload->V3 = (uint16_t)obj->valueint;
    obj = NULL;

    obj = cJSON_GetObjectItem(content, "V4");
    if (obj) payload->V4 = (uint16_t)obj->valueint;
    obj = NULL;

    obj = cJSON_GetObjectItem(content, "I");
    if (obj) payload->I = (int8_t)obj->valueint;
    obj = NULL;

    obj = cJSON_GetObjectItem(content, "I1");
    if (obj) payload->I1 = (int16_t)obj->valueint;
    obj = NULL;

    obj = cJSON_GetObjectItem(content, "I2");
    if (obj) payload->I2 = (int16_t)obj->valueint;
    obj = NULL;

    obj = cJSON_GetObjectItem(content, "I3");
    if (obj) payload->I3 = (int16_t)obj->valueint;
    obj = NULL;

    obj = cJSON_GetObjectItem(content, "I4");
    if (obj) payload->I4 = (int16_t)obj->valueint;
    obj = NULL;

    obj = cJSON_GetObjectItem(content, "aT");
    if (obj) payload->aT = (int16_t)obj->valueint;
    obj = NULL;

    obj = cJSON_GetObjectItem(content, "cT");
    if (obj) payload->cT = (int16_t)obj->valueint;
    obj = NULL;

    obj = cJSON_GetObjectItem(content, "T1");
    if (obj) payload->T1 = (int16_t)obj->valueint;
    obj = NULL;

    obj = cJSON_GetObjectItem(content, "T2");
    if (obj) payload->T2 = (int16_t)obj->valueint;
    obj = NULL;

    obj = cJSON_GetObjectItem(content, "T3");
    if (obj) payload->T3 = (int16_t)obj->valueint;
    obj = NULL;

    obj = cJSON_GetObjectItem(content, "T4");
    if (obj) payload->T4 = (int16_t)obj->valueint;
    obj = NULL;

    obj = cJSON_GetObjectItem(content, "OTC_threshold");
    if (obj) payload->OTC_threshold = (int16_t)obj->valueint;

    return payload;
}

size_t encode_frame(const uint8_t* input, size_t input_len, uint8_t* output) {
    /*
      The beginning and end of a radio message will be marked with 0x7E (decimal 126, char '~').
      This means any instances of 126 in the data should be 'escaped' - using 0x7D (decimal 125).
      Any instances of 125 should also be escaped.
      The replacements are:
          0x7E -> 0x7D 0x5E
          0x7D -> 0x7D 0x5D
    */

    size_t out_len = 0;
    output[out_len++] = FRAME_END; // start of frame

    for (size_t i = 0; i < input_len; ++i) {
        if (input[i] == FRAME_END) {
            output[out_len++] = FRAME_ESC;
            output[out_len++] = ESC_END;
        } else if (input[i] == FRAME_ESC) {
            output[out_len++] = FRAME_ESC;
            output[out_len++] = ESC_ESC;
        } else {
            output[out_len++] = input[i];
        }
    }

    output[out_len++] = FRAME_END; // end of frame
    return out_len;
}

size_t decode_frame(const uint8_t* input, size_t input_len, uint8_t* output) {
    /*
      Opposite to the encode_frame function:
          remove instances of 0x7E
          replace 0x7D 0x5E -> 0x7E
          replace 0x7D 0x5D -> 0x7D
    */
    size_t out_len = 0;
    bool in_escape = false;

    for (size_t i = 0; i < input_len; ++i) {
        uint8_t byte = input[i];

        if (byte == FRAME_END) {
            continue; // skip or mark frame boundaries
        } else if (byte == FRAME_ESC) {
            in_escape = true;
        } else {
            if (in_escape) {
                if (byte == ESC_END)
                    output[out_len++] = FRAME_END;
                else if (byte == ESC_ESC)
                    output[out_len++] = FRAME_ESC;
                else
                    ; // invalid escape, discard or handle
                in_escape = false;
            } else {
                output[out_len++] = byte;
            }
        }
    }

    return out_len;
}

void convert_from_binary(uint8_t* decoded_message) {
    uint8_t n_devices = decoded_message[0];
    cJSON* json_array = cJSON_CreateArray();
    if (json_array == NULL) {
        ESP_LOGE(TAG, "Failed to create JSON array");
        return;
    }

    for (uint8_t i=0; i<n_devices; i++) {
        radio_payload* payload = (radio_payload*)&decoded_message[1+i*sizeof(radio_payload)];
        cJSON* message = cJSON_CreateObject();
        if (message == NULL) {
            ESP_LOGE(TAG, "Failed to create JSON object");
            cJSON_Delete(json_array);
            return;
        }
        cJSON* content = cJSON_CreateObject();
        if (content == NULL) {
            ESP_LOGE(TAG, "Failed to create content object");
            cJSON_Delete(message);
            cJSON_Delete(json_array);
            return;
        }

        cJSON_AddNumberToObject(content, "Q", payload->Q);
        cJSON_AddNumberToObject(content, "H", payload->H);
        cJSON_AddNumberToObject(content, "V", payload->V);
        cJSON_AddNumberToObject(content, "V1", payload->V1);
        cJSON_AddNumberToObject(content, "V2", payload->V2);
        cJSON_AddNumberToObject(content, "V3", payload->V3);
        cJSON_AddNumberToObject(content, "V4", payload->V4);
        cJSON_AddNumberToObject(content, "I", payload->I);
        cJSON_AddNumberToObject(content, "I1", payload->I1);
        cJSON_AddNumberToObject(content, "I2", payload->I2);
        cJSON_AddNumberToObject(content, "I3", payload->I3);
        cJSON_AddNumberToObject(content, "I4", payload->I4);
        cJSON_AddNumberToObject(content, "aT", payload->aT);
        cJSON_AddNumberToObject(content, "cT", payload->cT);
        cJSON_AddNumberToObject(content, "T1", payload->T1);
        cJSON_AddNumberToObject(content, "T2", payload->T2);
        cJSON_AddNumberToObject(content, "T3", payload->T3);
        cJSON_AddNumberToObject(content, "T4", payload->T4);
        cJSON_AddNumberToObject(content, "OTC_threshold", payload->OTC_threshold);

        if (payload->type == 1) cJSON_AddStringToObject(message, "type", "data");

        char id_str[8]; // enough to hold "bms_255\0"
        snprintf(id_str, sizeof(id_str), "bms_%u", payload->esp_id);
        cJSON_AddStringToObject(message, "id", id_str);

        cJSON_AddItemToObject(message, "content", content);

        cJSON_AddItemToArray(json_array, message);
    }

    char* message_string = cJSON_PrintUnformatted(json_array);
    cJSON_Delete(json_array);
    if (message_string == NULL) {
        ESP_LOGE(TAG, "Failed to print cJSON to string\n");
        return;
    } else {
        send_message(message_string);
        free(message_string);
    }
}

void lora_task(void *pvParameters) {
    // receiver variables
    uint8_t RX_buffer[LORA_MAX_PACKET_LEN + 1];
    uint8_t RX_encoded_payload[(sizeof(radio_payload) + 1) * (MESH_SIZE + 1)];
    //                                   ^              ^            ^    ^
    //                               19 bytes       allow for       total number
    //                                 per          1 escaped       of devices in
    //                               payload      character per    mesh, including
    //                                           payload, on avg        self
    size_t RX_full_len = 0;
    bool RX_chunked = false;


    // transmitter variables
    char TX_individual_message[LORA_MAX_PACKET_LEN];
    uint8_t TX_n_devices = 1; // the transmitter, at least
    int64_t TX_delay_transmission_until = 0; // microseconds


    // setup
    lora_write_register(REG_FIFO_RX_BASE_ADDR, 0x00);
    lora_write_register(REG_FIFO_TX_BASE_ADDR, 0x00);
    lora_write_register(REG_FIFO_ADDR_PTR, 0x00);
    // clear IRQ flags
    lora_write_register(REG_IRQ_FLAGS, 0b11111111);


    // task loop
    while(true) {
        /*
            -------------
            RECEIVER CODE
            -------------
        */
        // enter continuous RX mode
        lora_write_register(REG_OP_MODE, 0b10000101); // LoRa + RX mode
        // wait for RX done (DIO0 goes high)
        if (gpio_get_level(PIN_NUM_DIO0) == 1) {
            uint8_t irq_flags = lora_read_register(REG_IRQ_FLAGS);
            if (irq_flags & 0x40) {  // RX_DONE
                uint8_t len = lora_read_register(0x13);  // RX bytes
                uint8_t fifo_rx_current = lora_read_register(0x10);
                lora_write_register(REG_FIFO_ADDR_PTR, fifo_rx_current);

                for (int i = 0; i < len; i++)
                    RX_buffer[i] = lora_read_register(0x00); // 0x00 = REG_FIFO ?


                if (!RX_chunked) {
                    if (RX_buffer[0] == FRAME_END) {
                        if (RX_buffer[len - 1] != FRAME_END) RX_chunked = true;
                    }
                    else {
                        // bogus message received
                        lora_write_register(REG_IRQ_FLAGS, 0b11111111);
                        continue;
                    }
                } else if (RX_chunked && RX_buffer[len - 1] == FRAME_END) RX_chunked = false;
                memcpy(&RX_encoded_payload[RX_full_len], RX_buffer, len);
                RX_full_len += len;

                if (!RX_chunked) {
                    uint8_t rssi_raw = lora_read_register(0x1A);
                    int rssi_dbm = -157 + rssi_raw;

                    uint8_t decoded_payload[RX_full_len];
                    size_t decoded_len = decode_frame(RX_encoded_payload, RX_full_len, decoded_payload);
                    RX_full_len = 0;

                    ESP_LOGI(TAG, "Received radio message with RSSI: %d dBm", rssi_dbm);
                    if (LORA_IS_RECEIVER) convert_from_binary(decoded_payload);
                    //
                    //
                    // else {
                    //      is a master node, so figure out
                    //      which mode the request is for then
                    //      perform_request(); etc.
                    // }
                    //
                }
            }

            // Clear IRQ flags again
            lora_write_register(REG_IRQ_FLAGS, 0b11111111);
        }


        /*
            ----------------
            TRANSMITTER CODE
            ----------------
        */
        if (esp_timer_get_time() > TX_delay_transmission_until) {
            if (LORA_IS_RECEIVER) {
                //
                //
                // forward requests made on the webserver
                // to all master nodes out there
                //
                //
                TX_delay_transmission_until = esp_timer_get_time();
            } else {
                // now form the LoRa message out of non-empty messages and transmit
                for (int i=0; i<MESH_SIZE; i++) {
                    if (strcmp(all_messages[i].id, "") != 0)
                        TX_n_devices++;
                }

                uint8_t combined_payload[1 + TX_n_devices * sizeof(radio_payload)];
                combined_payload[0] = TX_n_devices; // first bytes is number of radio_payloads to expect

                // get own data first
                char *data_string = get_data(false);
                snprintf(TX_individual_message, sizeof(TX_individual_message), "{\"type\":\"data\",\"id\":\"%s\",\"content\":%s}", ESP_ID, data_string);
                // add to radio payload
                radio_payload* payload;
                payload = convert_to_binary(TX_individual_message);
                if (payload == NULL) continue;
                memcpy(&combined_payload[1], (uint8_t*)payload, sizeof(radio_payload));
                free(payload);

                // now add the data of other devices in mesh to payload
                TX_n_devices = 1;
                for (int i=0; i<MESH_SIZE; i++) {
                    if (strcmp(all_messages[i].id, "") != 0) {
                        TX_n_devices++;

                        payload = convert_to_binary(all_messages[i].message);
                        if (payload == NULL) continue;
                        memcpy(&combined_payload[1 + (TX_n_devices-1) * sizeof(radio_payload)], (uint8_t*)payload, sizeof(radio_payload));
                        free(payload);

                        // clear the message slot again in case of disconnect
                        strcpy(all_messages[i].id, "");
                        all_messages[i].id[0] = '\0';
                        strcpy(all_messages[i].message, "");
                        all_messages[i].message[0] = '\0';
                    }
                }
                TX_n_devices = 1; // reset


                uint8_t encoded_combined_payload[(sizeof(radio_payload) + 1) * (MESH_SIZE + 1)];
                //                                         ^              ^            ^    ^
                //                                     19 bytes       allow for       total number
                //                                       per          1 escaped       of devices in
                //                                     payload      character per    mesh, including
                //                                                 payload, on avg        self
                size_t full_len = encode_frame(combined_payload, sizeof(combined_payload), encoded_combined_payload);
                uint8_t chunk[LORA_MAX_PACKET_LEN] = {0};
                for (int offset = 0; offset < full_len; offset += LORA_MAX_PACKET_LEN) {
                    int chunk_len = MIN(LORA_MAX_PACKET_LEN, full_len - offset);
                    memcpy(chunk, encoded_combined_payload + offset, chunk_len);

                    lora_write_register(REG_OP_MODE, 0b10000001); // LoRa + standby

                    // set DIO0 = TxDone
                    lora_write_register(REG_DIO_MAPPING_1, 0b01000000); // bits 7-6 for DIO0

                    lora_write_register(REG_FIFO_ADDR_PTR, 0x00); // reset FIFO pointer to base address

                    // write payload to FIFO
                    for (int i = 0; i < chunk_len; i++)
                        lora_write_register(REG_FIFO, chunk[i]);

                    lora_write_register(REG_PAYLOAD_LENGTH, chunk_len);

                    lora_write_register(REG_IRQ_FLAGS, 0b11111111); // clear all IRQ flags

                    lora_write_register(REG_OP_MODE, 0b10000011); // LoRa + TX mode

                    // wait for TX done (DIO0 goes high)
                    while (gpio_get_level(PIN_NUM_DIO0) == 0)
                        vTaskDelay(pdMS_TO_TICKS(1));

                    lora_write_register(REG_IRQ_FLAGS, 0b00001000);  // clear TxDone

                    vTaskDelay(pdMS_TO_TICKS(50)); // brief delay between chunks
                }
                int transmission_delay = calculate_transmission_delay(LORA_SF, LORA_BW, 8, full_len, LORA_CR, LORA_HEADER, LORA_LDRO);
                ESP_LOGI(TAG, "Radio packet sent. Delaying for %d ms", transmission_delay);

                TX_delay_transmission_until = (int64_t)(transmission_delay * 1000) + esp_timer_get_time();

                // set DIO0 = RxDone again
                lora_write_register(REG_DIO_MAPPING_1, 0b00000000); // bits 7-6 for DIO0
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
