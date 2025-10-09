#ifndef CONFIG_H
#define CONFIG_H

#include "sdkconfig.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef CONFIG_VERBOSE
    #define VERBOSE true
#else
    #define VERBOSE false
#endif
#ifdef CONFIG_DEV
    #define DEV true
#else
    #define DEV false
#endif
# define AZURE_URL CONFIG_AZURE_URL
#ifdef CONFIG_LOCAL
    #define LOCAL true
#else
    #define LOCAL false
#endif
# define FLASK_IP CONFIG_FLASK_IP

// I2C
#ifdef CONFIG_SCAN_I2C
    #define SCAN_I2C true
#else
    #define SCAN_I2C false
#endif
#define I2C_MASTER_TIMEOUT_MS     5000 // time delay to allow for BMS response
#define I2C_MASTER_NUM     I2C_NUM_0                 // I2C port number for master dev
#define I2C_ADDR           CONFIG_I2C_ADDR           // 7-bit I2C address of the battery
#define I2C_MASTER_SDA_IO  CONFIG_MASTER_SDA_PIN // GPIO number for I2C master data
#define I2C_MASTER_SCL_IO  CONFIG_MASTER_SCL_PIN // GPIO number for I2C master clock
#define I2C_MASTER_FREQ_HZ CONFIG_FREQ_HZ        // I2C master clock frequency
#define I2C_DELAY          CONFIG_DELAY          // I2C read / write delay
#define I2C_MANUFACTURER_ACCESS       CONFIG_MANUFACTURER_ACCESS
#define I2C_MANUFACTURER_BLOCK_ACCESS CONFIG_MANUFACTURER_BLOCK_ACCESS
#define I2C_RELATIVE_STATE_OF_CHARGE_ADDR CONFIG_RELATIVE_STATE_OF_CHARGE_ADDR
#define I2C_STATE_OF_HEALTH_ADDR          CONFIG_STATE_OF_HEALTH_ADDR
#define I2C_TEMPERATURE_ADDR              CONFIG_TEMPERATURE_ADDR
#define I2C_VOLTAGE_ADDR                  CONFIG_VOLTAGE_ADDR
#define I2C_CURRENT_ADDR                  CONFIG_CURRENT_ADDR
#define I2C_DEVICE_NAME_ADDR      CONFIG_DEVICE_NAME_ADDR
#define UTILS_ID_LENGTH 20
#define I2C_OPERATION_STATUS_ADDR CONFIG_OPERATION_STATUS_ADDR
#define I2C_DA_STATUS_1_ADDR      CONFIG_DA_STATUS_1_ADDR
#define I2C_DA_STATUS_2_ADDR      CONFIG_DA_STATUS_2_ADDR
#define I2C_OTC_THRESHOLD_ADDR    CONFIG_OTC_THRESHOLD_ADDR
#define BMS_RESET_CMD    CONFIG_RESET_CMD
#define BMS_SEAL_CMD     CONFIG_SEAL_CMD
#define BMS_UNSEAL_CMD_1 CONFIG_UNSEAL_CMD_1
#define BMS_UNSEAL_CMD_2 CONFIG_UNSEAL_CMD_2
#define BMS_FULL_ACCESS_CMD_1 CONFIG_FULL_ACCESS_CMD_1
#define BMS_FULL_ACCESS_CMD_2 CONFIG_FULL_ACCESS_CMD_2
#define UNIT_I2C_ADDR      CONFIG_UNIT_I2C_ADDR

// UART
#define GPS_UART_NUM UART_NUM_1
#define GPS_BUFF_SIZE (1024)
#define GPS_GPIO_NUM_32 32
#define GPS_GPIO_NUM_33 33

// WiFi:
#define DNS_PORT 53
#define     WIFI_SSID         CONFIG_WIFI_SSID
#define     WIFI_PASSWORD     CONFIG_WIFI_PASSWORD
#ifdef                        CONFIG_WIFI_AUTO_CONNECT
    #define WIFI_AUTO_CONNECT true
#else
    #define WIFI_AUTO_CONNECT false
#endif
#define AP_MAX_STA_CONN 4
#define WS_MAX_N_HTML_PAGES 1
#define WS_MAX_HTML_PAGE_NAME_LENGTH 32
#define WS_MAX_HTML_SIZE 700
#define WS_CONFIG_MAX_CLIENTS 3
#define WS_USERNAME CONFIG_USERNAME
#define WS_PASSWORD CONFIG_PASSWORD
#define WS_MESSAGE_MAX_LEN 512
#define WS_QUEUE_SIZE 10
#define UTILS_AUTH_TOKEN_LENGTH CONFIG_AUTH_TOKEN_LENGTH
#define MESH_SIZE 5
#define MESH_MAX_HTTP_RECV_BUFFER 128

typedef struct {
    int descriptor;
    char auth_token[UTILS_AUTH_TOKEN_LENGTH];
    bool is_browser_not_mesh;
    uint8_t esp_id; // just the number following "bms_", only relevent for mesh ws clients
} client_socket;

// LoRa:
#ifdef CONFIG_IS_RECEIVER
    #define LORA_IS_RECEIVER true
#else
    #define LORA_IS_RECEIVER false
#endif
#define LORA_MAX_PACKET_LEN 255
#define PIN_NUM_MISO CONFIG_SPI_MISO_PIN
#define PIN_NUM_MOSI CONFIG_SPI_MOSI_PIN
#define PIN_NUM_CLK  CONFIG_SPI_SCK_PIN
#define PIN_NUM_CS   CONFIG_SPI_NSS_CS_PIN
#define PIN_NUM_RST  CONFIG_SPI_RST_PIN
#define PIN_NUM_DIO0 CONFIG_SPI_DIO0_PIN
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
#define LORA_OUTPUT_POWER CONFIG_OUTPUT_POWER
#ifdef CONFIG_POWER_BOOST
    #define LORA_POWER_BOOST true
#else
    #define LORA_POWER_BOOST false
#endif
#define REG_FIFO                0x00
#define REG_OP_MODE             0x01
#define REG_FRF_MSB             0x06
#define REG_FRF_MID             0x07
#define REG_FRF_LSB             0x08
#define REG_PA_CONFIG           0x09
#define REG_PA_DAC              0x4D
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
#define MODE_SLEEP         0b00000000
#define MODE_STDBY         0b00000001
#define MODE_LORA          0b10000000

typedef struct {
    uint8_t esp_id;
    char message[WS_MESSAGE_MAX_LEN];
} LoRa_message;


typedef struct {
    int stack_size;
    const char* task_name;
} TaskParams;

#endif // CONFIG_H
