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
#define I2C_RELATIVE_STATE_OF_CHARGE_ADDR CONFIG_RELATIVE_STATE_OF_CHARGE_ADDR

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
