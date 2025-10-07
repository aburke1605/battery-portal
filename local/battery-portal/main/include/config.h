#ifndef CONFIG_H
#define CONFIG_H

#define AP_MAX_STA_CONN 4

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
#define I2C_OPERATION_STATUS_ADDR CONFIG_OPERATION_STATUS_ADDR
#define BMS_RESET_CMD    CONFIG_RESET_CMD
#define BMS_SEAL_CMD     CONFIG_SEAL_CMD
#define BMS_UNSEAL_CMD_1 CONFIG_UNSEAL_CMD_1
#define BMS_UNSEAL_CMD_2 CONFIG_UNSEAL_CMD_2
#define BMS_FULL_ACCESS_CMD_1 CONFIG_FULL_ACCESS_CMD_1
#define BMS_FULL_ACCESS_CMD_2 CONFIG_FULL_ACCESS_CMD_2

// UART
#define GPS_UART_NUM UART_NUM_1
#define GPS_BUFF_SIZE (1024)
#define GPS_GPIO_NUM_32 32
#define GPS_GPIO_NUM_33 33

// WiFi:
#define     WIFI_SSID         CONFIG_WIFI_SSID
#define     WIFI_PASSWORD     CONFIG_WIFI_PASSWORD
#ifdef                        CONFIG_WIFI_AUTO_CONNECT
    #define WIFI_AUTO_CONNECT true
#else
    #define WIFI_AUTO_CONNECT false
#endif

// LoRa:
#ifdef CONFIG_IS_RECEIVER
    #define LORA_IS_RECEIVER true
#else
    #define LORA_IS_RECEIVER false
#endif

#endif // CONFIG_H
