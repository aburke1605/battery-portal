#include "include/LoRa.h"

#include "include/config.h"
#include "include/utils.h"
#include "include/SPI.h"

#include <stdint.h>
#include "esp_log.h"

static const char* TAG = "LoRa";

void lora_init() {
    spi_init();

    // Set carrier frequency
    uint64_t frf = ((uint64_t)(LORA_FREQ * 1E6) << 19) / 32000000;
    spi_write_register(REG_FRF_MSB, (uint8_t)(frf >> 16));
    spi_write_register(REG_FRF_MID, (uint8_t)(frf >> 8));
    spi_write_register(REG_FRF_LSB, (uint8_t)(frf >> 0));

    // Set LNA gain to maximum
    spi_write_register(REG_LNA, 0b00100011);  // LNA_MAX_GAIN | LNA_BOOST

    // Enable AGC (bit 2 of RegModemConfig3)
    spi_write_register(REG_MODEM_CONFIG_3,                                                                    // bits:
        (0b00001111 & 0)                                                                               << 4 |  //  7-4
        (0b00000001 & (LORA_LDRO | (calculate_symbol_length(LORA_SF, LORA_BW) > 16.0 ? true : false))) << 3 |  //  3
        (0b00000001 & 1)                                                                               << 2 |  //  2    AGC on
        (0b00000011 & 0)                                                                                       //  1-0
    );

    // Configure modem parameters:
    //  bandwidth, coding rate, header
    spi_write_register(REG_MODEM_CONFIG_1,  // bits:
        (0b00001111 & LORA_BW)     << 4 |    //  7-4
        (0b00000111 & LORA_CR)     << 1 |    //  3-1
        (0b00000001 & LORA_HEADER)           //  0
    );
    //  spreading factor, cyclic redundancy check
    spi_write_register(REG_MODEM_CONFIG_2,     // bits:
        (0b00001111 & LORA_SF)          << 4 |  //  7-4
        (0b00000001 & LORA_Tx_CONT)     << 3 |  //  3
        (0b00000001 & LORA_Rx_PAYL_CRC) << 2 |  //  2
        (0b00000011 & 0)                        //  1-0
    );  // SF7, TxContinuousMode=0, CRC on

    // Preamble length (8 bytes = 0x0008)
    spi_write_register(REG_PREAMBLE_MSB, 0x00);
    spi_write_register(REG_PREAMBLE_LSB, 0x08);

    // Set output power to 13 dBm using PA_BOOST
    spi_write_register(REG_PA_CONFIG,           // bits:
        (0b00000001 & 1)                 << 7 |  // 7   PA BOOST
        (0b00000111 & 0x04)              << 4 |  // 6-4
        (0b00001111 & LORA_OUTPUT_POWER)
    );

    if (LORA_POWER_BOOST) spi_write_register(REG_PA_DAC, 0x87);
    else spi_write_register(REG_PA_DAC, 0x84);

    ESP_LOGI(TAG, "SX127x configured to RadioHead defaults");
}
