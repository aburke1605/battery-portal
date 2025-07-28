#include "include/LoRa.h"

#include "include/BMS.h"
#include "include/global.h"
#include "include/WS.h"
#include "include/utils.h"

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

size_t json_to_binary(uint8_t* binary_message, cJSON* json_array) {
    // build binary_message from json_array
    uint8_t n_devices = 0;
    size_t packet_start = 1; // after first byte n_devices
    cJSON* item = NULL;
    cJSON_ArrayForEach(item, json_array) {
        if (!cJSON_IsObject(item)) return 0;

        cJSON* type = cJSON_GetObjectItem(item, "type");
        if (!type) {
            ESP_LOGE(TAG, "No \"type\" key in cJSON array item");
            return 0;
        }

        cJSON* content = cJSON_GetObjectItem(item, "content");
        if (!content) {
            ESP_LOGE(TAG, "No \"content\" key in cJSON array item");
            return 0;
        }


        if (strcmp(type->valuestring, "data") == 0) {
            radio_data_packet* packet = malloc(sizeof(radio_data_packet));
            if (packet == NULL) {
                ESP_LOGE(TAG, "Failed to allocate packet memory\n");
                return 0;
            }
            memset(packet, 0, sizeof(radio_data_packet));
            packet->type = DATA;

            cJSON *id = cJSON_GetObjectItem(item, "id");
            if (!id) {
                ESP_LOGE(TAG, "No \"id\" key in cJSON array item");
                return 0;
            }

            if (strcmp(id->valuestring, "unknown") == 0)
                packet->esp_id = 0;
            else {
                char esp_id_number[3];
                strcpy(esp_id_number, &id->valuestring[4]);
                packet->esp_id = atoi(esp_id_number);
            }

            cJSON *content = cJSON_GetObjectItem(item, "content");
            if (!content) {
                ESP_LOGE(TAG, "No \"content\" key in cJSON array item");
                return 0;
            }
            cJSON *obj;

            obj = cJSON_GetObjectItem(content, "Q");
            if (obj) packet->Q = (uint8_t)obj->valueint;
            obj = NULL;

            obj = cJSON_GetObjectItem(content, "H");
            if (obj) packet->H = (uint8_t)obj->valueint;
            obj = NULL;

            obj = cJSON_GetObjectItem(content, "V");
            if (obj) packet->V = (uint8_t)obj->valueint;
            obj = NULL;

            obj = cJSON_GetObjectItem(content, "V1");
            if (obj) packet->V1 = (uint16_t)obj->valueint;
            obj = NULL;

            obj = cJSON_GetObjectItem(content, "V2");
            if (obj) packet->V2 = (uint16_t)obj->valueint;
            obj = NULL;

            obj = cJSON_GetObjectItem(content, "V3");
            if (obj) packet->V3 = (uint16_t)obj->valueint;
            obj = NULL;

            obj = cJSON_GetObjectItem(content, "V4");
            if (obj) packet->V4 = (uint16_t)obj->valueint;
            obj = NULL;

            obj = cJSON_GetObjectItem(content, "I");
            if (obj) packet->I = (int8_t)obj->valueint;
            obj = NULL;

            obj = cJSON_GetObjectItem(content, "I1");
            if (obj) packet->I1 = (int16_t)obj->valueint;
            obj = NULL;

            obj = cJSON_GetObjectItem(content, "I2");
            if (obj) packet->I2 = (int16_t)obj->valueint;
            obj = NULL;

            obj = cJSON_GetObjectItem(content, "I3");
            if (obj) packet->I3 = (int16_t)obj->valueint;
            obj = NULL;

            obj = cJSON_GetObjectItem(content, "I4");
            if (obj) packet->I4 = (int16_t)obj->valueint;
            obj = NULL;

            obj = cJSON_GetObjectItem(content, "aT");
            if (obj) packet->aT = (int16_t)obj->valueint;
            obj = NULL;

            obj = cJSON_GetObjectItem(content, "cT");
            if (obj) packet->cT = (int16_t)obj->valueint;
            obj = NULL;

            obj = cJSON_GetObjectItem(content, "T1");
            if (obj) packet->T1 = (int16_t)obj->valueint;
            obj = NULL;

            obj = cJSON_GetObjectItem(content, "T2");
            if (obj) packet->T2 = (int16_t)obj->valueint;
            obj = NULL;

            obj = cJSON_GetObjectItem(content, "T3");
            if (obj) packet->T3 = (int16_t)obj->valueint;
            obj = NULL;

            obj = cJSON_GetObjectItem(content, "T4");
            if (obj) packet->T4 = (int16_t)obj->valueint;
            obj = NULL;

            obj = cJSON_GetObjectItem(content, "OTC_threshold");
            if (obj) packet->OTC_threshold = (int16_t)obj->valueint;


            // now copy the packet into the returned binary binary_message which will be broadcasted
            memcpy(&binary_message[packet_start], packet, sizeof(radio_data_packet));
            // and shift the position along for the next iteration
            packet_start += sizeof(radio_data_packet);
        }

        else if (strcmp(type->valuestring, "request") == 0) {
            radio_request_packet* packet = malloc(sizeof(radio_request_packet));
            if (packet == NULL) {
                ESP_LOGE(TAG, "Failed to allocate packet memory\n");
                return 0;
            }
            memset(packet, 0, sizeof(radio_request_packet));
            packet->type = REQUEST;

            cJSON* esp_id = cJSON_GetObjectItem(item, "esp_id");
            if (!esp_id) {
                ESP_LOGE(TAG, "No \"esp_id\" key in cJSON array item");
                return 0;
            }
            packet->esp_id = atoi(&esp_id->valuestring[4]);


            cJSON* summary = cJSON_GetObjectItem(content, "summary");
            if (!summary) {
                ESP_LOGE(TAG, "No \"summary\" key in cJSON array item");
                return 0;
            }
            if (strcmp(summary->valuestring, "change-settings") == 0) {
                cJSON* data = cJSON_GetObjectItem(content, "data");
                if (!data) {
                    ESP_LOGE(TAG, "No \"data\" key in cJSON array item");
                    return 0;
                }
                packet->request = CHANGE_SETTINGS;

                cJSON* new_esp_id = cJSON_GetObjectItem(data, "new_esp_id");
                if (new_esp_id) packet->new_esp_id = new_esp_id->valueint;

                cJSON* OTC_threshold = cJSON_GetObjectItem(data, "OTC_threshold");
                if (OTC_threshold) packet->OTC_threshold = OTC_threshold->valueint;
            }
            else if (strcmp(summary->valuestring, "connect-wifi") == 0) {
                cJSON* data = cJSON_GetObjectItem(content, "data");
                if (!data) {
                    ESP_LOGE(TAG, "No \"data\" key in cJSON array item");
                    return 0;
                }
                packet->request = CONNECT_WIFI;

                cJSON* eduroam = cJSON_GetObjectItem(data, "eduroam");
                if (!eduroam) {
                    ESP_LOGE(TAG, "No \"eduroam\" key in cJSON array item");
                    return 0;
                }
                cJSON* username = cJSON_GetObjectItem(data, "username");
                if (!username) {
                    ESP_LOGE(TAG, "No \"username\" key in cJSON array item");
                    return 0;
                }
                cJSON* password = cJSON_GetObjectItem(data, "password");
                if (!password) {
                    ESP_LOGE(TAG, "No \"password\" key in cJSON array item");
                    return 0;
                }
                packet->eduroam = (bool)eduroam->valueint;
                for (size_t i=0; i<sizeof(packet->username); i++) packet->username[i] = (uint8_t)username->valuestring[i];
                for (size_t i=0; i<sizeof(packet->password); i++) packet->password[i] = (uint8_t)password->valuestring[i];
            }
            else if (strcmp(summary->valuestring, "reset-bms") == 0) packet->request = RESET_BMS;
            else if (strcmp(summary->valuestring, "unseal-bms") == 0) packet->request = UNSEAL_BMS;


            // now copy the packet into the returned binary binary_message which will be broadcasted
            memcpy(&binary_message[packet_start], packet, sizeof(radio_request_packet));
            // and shift the position along for the next iteration
            packet_start += sizeof(radio_request_packet);
        }



        n_devices++;
    }

    binary_message[0] = n_devices;

    return packet_start;
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

void binary_to_json(uint8_t* binary_message, cJSON* json_array) {
    // build json_array from binary_message
    uint8_t n_devices = binary_message[0];
    size_t packet_start = 1; // after first byte n_devices
    for (uint8_t i=0; i<n_devices; i++) {
        uint8_t type = binary_message[packet_start];
        cJSON* message = cJSON_CreateObject();
        if (message == NULL) {
            ESP_LOGE(TAG, "Failed to create JSON object");
            cJSON_Delete(json_array);
            return;
        }

        char id_str[8]; // enough to hold "bms_255\0"

        if (type == DATA) {
            radio_data_packet* packet = (radio_data_packet*)&binary_message[packet_start];
            cJSON_AddStringToObject(message, "type", "data");

            cJSON* content = cJSON_CreateObject();
            if (content == NULL) {
                ESP_LOGE(TAG, "Failed to create content object");
                cJSON_Delete(message);
                cJSON_Delete(json_array);
                return;
            }
            cJSON_AddNumberToObject(content, "Q", packet->Q);
            cJSON_AddNumberToObject(content, "H", packet->H);
            cJSON_AddNumberToObject(content, "V", packet->V);
            cJSON_AddNumberToObject(content, "V1", packet->V1);
            cJSON_AddNumberToObject(content, "V2", packet->V2);
            cJSON_AddNumberToObject(content, "V3", packet->V3);
            cJSON_AddNumberToObject(content, "V4", packet->V4);
            cJSON_AddNumberToObject(content, "I", packet->I);
            cJSON_AddNumberToObject(content, "I1", packet->I1);
            cJSON_AddNumberToObject(content, "I2", packet->I2);
            cJSON_AddNumberToObject(content, "I3", packet->I3);
            cJSON_AddNumberToObject(content, "I4", packet->I4);
            cJSON_AddNumberToObject(content, "aT", packet->aT);
            cJSON_AddNumberToObject(content, "cT", packet->cT);
            cJSON_AddNumberToObject(content, "T1", packet->T1);
            cJSON_AddNumberToObject(content, "T2", packet->T2);
            cJSON_AddNumberToObject(content, "T3", packet->T3);
            cJSON_AddNumberToObject(content, "T4", packet->T4);
            cJSON_AddNumberToObject(content, "OTC_threshold", packet->OTC_threshold);

            snprintf(id_str, sizeof(id_str), "bms_%u", packet->esp_id);
            cJSON_AddStringToObject(message, "id", id_str);

            cJSON_AddItemToObject(message, "content", content);

            cJSON_AddItemToArray(json_array, message);

            packet_start += sizeof(radio_data_packet);
        }

        else if(type == QUERY) {
            radio_query_packet* packet = (radio_query_packet*)&binary_message[packet_start];
            cJSON_AddStringToObject(message, "type", "query");

            snprintf(id_str, sizeof(id_str), "bms_%u", packet->esp_id);
            cJSON_AddStringToObject(message, "id", id_str);

            if (packet->query == -1) cJSON_AddStringToObject(message, "content", "are you still there?");

            cJSON_AddItemToArray(json_array, message);

            packet_start += sizeof(radio_query_packet);
        }

        else if(type == REQUEST) {
            radio_request_packet* packet = (radio_request_packet*)&binary_message[packet_start];
            cJSON_AddStringToObject(message, "type", "request");

            snprintf(id_str, sizeof(id_str), "bms_%03d", packet->esp_id);
            cJSON_AddStringToObject(message, "id", id_str);

            cJSON* content = cJSON_CreateObject();
            if (content == NULL) {
                ESP_LOGE(TAG, "Failed to create content object");
                cJSON_Delete(message);
                cJSON_Delete(json_array);
                return;
            }
            cJSON* data = cJSON_CreateObject();
            if (data == NULL) {
                ESP_LOGE(TAG, "Failed to create data object");
                cJSON_Delete(message);
                cJSON_Delete(content);
                cJSON_Delete(json_array);
                return;
            }

            if (packet->request == CHANGE_SETTINGS) {
                cJSON_AddStringToObject(content, "summary", "change-settings");

                char new_id_str[8]; // enough to hold "bms_255\0"
                snprintf(new_id_str, sizeof(new_id_str), "bms_%03u", packet->new_esp_id);
                cJSON_AddStringToObject(data, "new_esp_id", new_id_str);

                cJSON_AddNumberToObject(data, "OTC_threshold", packet->OTC_threshold);

                cJSON_AddItemToObject(content, "data", data);
            }
            else if (packet->request == CONNECT_WIFI) {
                cJSON_AddStringToObject(content, "summary", "connect-wifi");

                cJSON_AddBoolToObject(data, "eduroam", packet->eduroam);

                cJSON_AddStringToObject(data, "username", (char*)packet->username);
                cJSON_AddStringToObject(data, "password", (char*)packet->password);

                cJSON_AddItemToObject(content, "data", data);
            }
            else if (packet->request == RESET_BMS) cJSON_AddStringToObject(content, "summary", "reset-bms");
            else if (packet->request == UNSEAL_BMS) cJSON_AddStringToObject(content, "summary", "unseal-bms");

            cJSON_AddItemToObject(message, "content", content);

            cJSON_AddItemToArray(json_array, message);

            packet_start += sizeof(radio_request_packet);
        }
    }
}

void lora_task(void *pvParameters) {
    // receiver variables
    uint8_t RX_buffer[5 * LORA_MAX_PACKET_LEN]; // this length likely needs to be changed
    uint8_t RX_encoded_payload[5 * LORA_MAX_PACKET_LEN]; // this length likely needs to be changed
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
                    cJSON* json_array = cJSON_CreateArray();
                    if (json_array == NULL) {
                        ESP_LOGE(TAG, "Failed to create JSON array");
                        return;
                    }
                    binary_to_json(decoded_payload, json_array);
                    char* message_string = malloc(WS_MESSAGE_MAX_LEN);
                    snprintf(message_string, WS_MESSAGE_MAX_LEN, "%s", cJSON_PrintUnformatted(json_array));
                    if (message_string == NULL) {
                        ESP_LOGE(TAG, "Failed to print cJSON to string\n");
                        return;
                    } else {
                        if (LORA_IS_RECEIVER) {
                            if (VERBOSE) {
                                ESP_LOGI(TAG, "Forwarding message:");
                                ESP_LOGI(TAG, "%s", message_string);
                                ESP_LOGI(TAG, "on to web server");
                            }
                            send_message(message_string);
                        } else {
                            if (VERBOSE) {
                                ESP_LOGI(TAG, "Processing messaged received from web server:");
                                ESP_LOGI(TAG, "%s", message_string);
                            }
                            cJSON* message = NULL;
                            cJSON_ArrayForEach(message, json_array) {
                                if (!cJSON_IsObject(message)) continue;

                                // pop the "id" key
                                cJSON* esp_id = cJSON_DetachItemFromObject(message, "id");
                                if (esp_id) {
                                    uint8_t id_int = atoi(&esp_id->valuestring[4]);
                                    if (id_int == ESP_ID) {
                                        if (VERBOSE) ESP_LOGI(TAG, "This request is for me, the mesh ROOT");
                                        cJSON *response = cJSON_CreateObject();
                                        perform_request(message, response);
                                    } else {
                                        if (VERBOSE) ESP_LOGI(TAG, "This request is for mesh client %s:", esp_id->valuestring);
                                        char* remainder_string = cJSON_PrintUnformatted(message);
                                        if (remainder_string != NULL) {
                                            if (VERBOSE) ESP_LOGI(TAG, "%s", remainder_string);
                                            // send to correct WebSocket client
                                            for (int i = 0; i < WS_CONFIG_MAX_CLIENTS; i++) {
                                                if (!client_sockets[i].is_browser_not_mesh && client_sockets[i].descriptor >= 0 && client_sockets[i].esp_id == id_int) {
                                                    httpd_ws_frame_t ws_pkt = {
                                                        .payload = (uint8_t *)remainder_string,
                                                        .len = strlen(remainder_string),
                                                        .type = HTTPD_WS_TYPE_TEXT,
                                                    };

                                                    int tries = 0;
                                                    int max_tries = 5;
                                                    esp_err_t err;
                                                    while (tries < max_tries) {
                                                        err = httpd_ws_send_frame_async(server, client_sockets[i].descriptor, &ws_pkt);
                                                        if (err == ESP_OK) break;
                                                        vTaskDelay(pdMS_TO_TICKS(1000));
                                                        tries++;
                                                    }

                                                    if (err != ESP_OK) {
                                                        ESP_LOGE(TAG, "Failed to send frame to client %d: %s", client_sockets[i].descriptor, esp_err_to_name(err));
                                                        remove_client(client_sockets[i].descriptor);  // Clean up disconnected clients
                                                    }
                                                }
                                            }
                                            free(remainder_string);
                                        }
                                    }
                                    cJSON_Delete(esp_id);
                                }
                            }
                        }
                        free(message_string);
                    }
                    cJSON_Delete(json_array);
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
                if (strcmp(forwarded_message, "") != 0) {
                    // forward requests made on the webserver
                    // to all master nodes out there
                    if (VERBOSE) {
                        ESP_LOGI(TAG, "Now forwarding:");
                        ESP_LOGI(TAG, "%s", forwarded_message);
                        ESP_LOGI(TAG, "by radio transmission");
                    }
                    cJSON *json_array = cJSON_Parse(forwarded_message);
                    if (json_array) {
                        uint8_t binary_message[5 * LORA_MAX_PACKET_LEN]; // this length likely needs to be changed
                        size_t binary_message_length = json_to_binary(binary_message, json_array);
                        if (binary_message_length > 0) {
                            uint8_t encoded_forwarded_message[binary_message_length];
                            size_t full_len = encode_frame(binary_message, binary_message_length, encoded_forwarded_message);
                            // size_t full_len = encode_frame(test, sizeof(test), encoded_forwarded_message);

                            lora_write_register(REG_OP_MODE, 0b10000001); // LoRa + standby

                            // set DIO0 = TxDone
                            lora_write_register(REG_DIO_MAPPING_1, 0b01000000); // bits 7-6 for DIO0

                            lora_write_register(REG_FIFO_ADDR_PTR, 0x00); // reset FIFO pointer to base address

                            // write payload to FIFO
                            for (int i = 0; i < full_len; i++)
                                lora_write_register(REG_FIFO, encoded_forwarded_message[i]);

                            lora_write_register(REG_PAYLOAD_LENGTH, full_len);

                            lora_write_register(REG_IRQ_FLAGS, 0b11111111); // clear all IRQ flags

                            lora_write_register(REG_OP_MODE, 0b10000011); // LoRa + TX mode

                            // wait for TX done (DIO0 goes high)
                            while (gpio_get_level(PIN_NUM_DIO0) == 0)
                                vTaskDelay(pdMS_TO_TICKS(1));

                            lora_write_register(REG_IRQ_FLAGS, 0b00001000);  // clear TxDone

                            int transmission_delay = calculate_transmission_delay(LORA_SF, LORA_BW, 8, full_len, LORA_CR, LORA_HEADER, LORA_LDRO);
                            ESP_LOGI(TAG, "Radio packet sent. Delaying for %d ms", transmission_delay);

                            TX_delay_transmission_until = (int64_t)(transmission_delay * 1000) + esp_timer_get_time();

                            // set DIO0 = RxDone again
                            lora_write_register(REG_DIO_MAPPING_1, 0b00000000); // bits 7-6 for DIO0
                        }
                    }
                    strcpy(forwarded_message, "\0");
                }
            } else {
                // now form the LoRa message out of non-empty messages and transmit
                for (int i=0; i<MESH_SIZE; i++) {
                    if (strcmp(all_messages[i].id, "") != 0)
                        TX_n_devices++;
                }

                cJSON* json_array = cJSON_CreateArray();
                if (json_array == NULL) {
                    ESP_LOGE(TAG, "Failed to create JSON array");
                    return;
                }

                // get own data first
                char *data_string = get_data(false);
                char* esp_id = esp_id_string();
                snprintf(TX_individual_message, sizeof(TX_individual_message), "{\"type\":\"data\",\"id\":\"%s\",\"content\":%s}", esp_id, data_string);
                free(esp_id);
                cJSON *item = cJSON_Parse(TX_individual_message);
                cJSON_AddItemToArray(json_array, item);

                // now add the data of other devices in mesh to payload
                TX_n_devices = 1;
                for (int i=0; i<MESH_SIZE; i++) {
                    if (strcmp(all_messages[i].id, "") != 0) {
                        TX_n_devices++;

                        item = cJSON_Parse(all_messages[i].message);
                        cJSON_AddItemToArray(json_array, item);

                        // clear the message slot again in case of disconnect
                        strcpy(all_messages[i].id, "");
                        all_messages[i].id[0] = '\0';
                        strcpy(all_messages[i].message, "");
                        all_messages[i].message[0] = '\0';
                    }
                }
                TX_n_devices = 1; // reset

                if (VERBOSE) {
                    ESP_LOGI(TAG, "ROOT: now transmitting:");
                    ESP_LOGI(TAG, "%s", cJSON_PrintUnformatted(json_array));
                    ESP_LOGI(TAG, "to receiver");
                }

                uint8_t binary_message[5 * LORA_MAX_PACKET_LEN]; // this length likely needs to be changed
                size_t binary_message_length = json_to_binary(binary_message, json_array);
                if (binary_message_length > 0) {

                    uint8_t encoded_combined_payload[binary_message_length];
                    size_t full_len = encode_frame(binary_message, binary_message_length, encoded_combined_payload);
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
                cJSON_Delete(json_array);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
