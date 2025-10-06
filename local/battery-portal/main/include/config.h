#ifndef CONFIG_H
#define CONFIG_H

#define AP_MAX_STA_CONN 4

// I2C
#define I2C_MASTER_TIMEOUT_MS     5000 // time delay to allow for BMS response
#define I2C_ADDR           CONFIG_I2C_ADDR           // 7-bit I2C address of the battery
#define I2C_RELATIVE_STATE_OF_CHARGE_ADDR CONFIG_RELATIVE_STATE_OF_CHARGE_ADDR

// LoRa:
#ifdef CONFIG_IS_RECEIVER
    #define LORA_IS_RECEIVER true
#else
    #define LORA_IS_RECEIVER false
#endif

#endif // CONFIG_H
