#ifndef CONFIG_H
#define CONFIG_H


#include "sdkconfig.h"

#include <stdbool.h>

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


#define AP_WIFI_SSID "AceOn battery"
#define AP_MAX_STA_CONN 4


#define DNS_PORT 53


#ifdef CONFIG_SCAN_I2C
    #define SCAN_I2C true
#else
    #define SCAN_I2C false
#endif
#define I2C_ADDR           CONFIG_I2C_ADDR           // 7-bit I2C address of the battery
#define I2C_MASTER_SDA_IO  CONFIG_I2C_MASTER_SDA_PIN // GPIO number for I2C master data
#define I2C_MASTER_SCL_IO  CONFIG_I2C_MASTER_SCL_PIN // GPIO number for I2C master clock
#define I2C_MASTER_FREQ_HZ CONFIG_I2C_FREQ_HZ        // I2C master clock frequency
#define I2C_DELAY          CONFIG_I2C_DELAY          // I2C read / write delay
#define I2C_MASTER_NUM     I2C_NUM_0                 // I2C port number for master dev

#define I2C_STATE_OF_CHARGE_REG CONFIG_STATE_OF_CHARGE_REG // Register address for StateOfCharge
#define I2C_STATE_OF_HEALTH_REG CONFIG_STATE_OF_HEALTH_REG // Register address for StateOfHealth
#define I2C_VOLTAGE_REG CONFIG_VOLTAGE_REG                 // Register address for Voltage
#define I2C_CURRENT_REG CONFIG_CURRENT_REG                 // Register address for AverageCurrent
#define I2C_TEMPERATURE_REG CONFIG_TEMPERATURE_REG         // Register address for Temperature
#define I2C_INT_TEMPERATURE_REG CONFIG_INT_TEMPERATURE_REG // Register address for InternalTemperature

#define I2C_CONTROL_REG             CONFIG_I2C_CONTROL_REG             // Control register address
#define I2C_CONTROL_RESET_SUBCMD    CONFIG_I2C_CONTROL_RESET_SUBCMD    // Reset subcommand
#define I2C_CONTROL_UNSEAL_SUBCMD_1 CONFIG_I2C_CONTROL_UNSEAL_SUBCMD_1 // UNSEAL subcommand part 1
#define I2C_CONTROL_UNSEAL_SUBCMD_2 CONFIG_I2C_CONTROL_UNSEAL_SUBCMD_2 // UNSEAL subcommand part 2

#define I2C_I2C_MASTER_PORT         0      // I2C port number (adjust as needed)

#define I2C_DATA_FLASH_CLASS       0x3E
#define I2C_DATA_FLASH_BLOCK       0x3F
#define I2C_BLOCK_DATA_START       0x40
#define I2C_BLOCK_DATA_CHECKSUM    0x60
#define I2C_BLOCK_DATA_CONTROL     0x61

#define I2C_MASTER_TIMEOUT_MS     1000 // time delay to allow for BMS response
#define I2C_MASTER_TX_BUF_DISABLE 0    // I2C master doesn't need buffer
#define I2C_MASTER_RX_BUF_DISABLE 0    // I2C master doesn't need buffer

#define I2C_CHARGE_INHIBIT_CFG_SUBCLASS_ID 32
#define I2C_CHG_INHIBIT_TEMP_LOW_OFFSET      0
#define I2C_CHG_INHIBIT_TEMP_HIGH_OFFSET     2

#define I2C_DATA_SUBCLASS_ID 48
#define I2C_NAME_OFFSET        46 // this is 45 in the manual but for unknown reason it's 46 that works

#define I2C_DISCHARGE_SUBCLASS_ID 49
#define I2C_BL_OFFSET                9
#define I2C_BH_OFFSET               14

#define I2C_CURRENT_THRESHOLDS_SUBCLASS_ID 81
#define I2C_DSG_CURRENT_THRESHOLD_OFFSET     0
#define I2C_CHG_CURRENT_THRESHOLD_OFFSET     2

#define I2C_LED_GPIO_PIN 2


#define UTILS_ID_LENGTH 10
#define UTILS_AUTH_TOKEN_LENGTH CONFIG_AUTH_TOKEN_LENGTH

#define UTILS_ROUTER_SSID CONFIG_ROUTER_SSID
#define UTILS_ROUTER_PASSWORD CONFIG_ROUTER_PASSWORD
#ifdef CONFIG_EDUROAM
    #define UTILS_EDUROAM true
#else
    #define UTILS_EDUROAM false
#endif
#define UTILS_EDUROAM_USERNAME CONFIG_EDUROAM_USERNAME
#define UTILS_EDUROAM_PASSWORD CONFIG_EDUROAM_PASSWORD


#define WS_USERNAME CONFIG_USERNAME
#define WS_PASSWORD CONFIG_PASSWORD

#define WS_QUEUE_SIZE 20
#define WS_MESSAGE_MAX_LEN 256

#define WS_MAX_HTML_SIZE 7500
#define WS_MAX_N_HTML_PAGES 16
#define WS_MAX_HTML_PAGE_NAME_LENGTH 32
#define WS_CONFIG_MAX_CLIENTS 3

#define MESH_MAX_HTTP_RECV_BUFFER 128
#define MESH_SIZE 10


// LoRa:
#ifdef CONFIG_IS_RECEIVER
    #define LORA_IS_RECEIVER true
#else
    #define LORA_IS_RECEIVER false
#endif

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
#define LORA_OUTPUT_POWER CONFIG_OUTPUT_POWER
#ifdef CONFIG_POWER_BOOST
    #define LORA_POWER_BOOST true
#else
    #define LORA_POWER_BOOST false
#endif

// register addresses
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

#define LORA_MAX_PACKET_LEN 255

// modes
#define MODE_SLEEP         0b00000000
#define MODE_STDBY         0b00000001
#define MODE_LORA          0b10000000

typedef struct {
    int descriptor;
    char auth_token[UTILS_AUTH_TOKEN_LENGTH];
    bool is_browser_not_mesh;
} client_socket;

typedef struct {
    char id[10];
    char message[WS_MESSAGE_MAX_LEN];
} LoRa_message;

typedef struct {
    int stack_size;
    const char* task_name;
} TaskParams;


#endif // CONFIG_H
