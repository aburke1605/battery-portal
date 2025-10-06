#ifndef CONFIG_H
#define CONFIG_H

#define AP_MAX_STA_CONN 4

// LoRa:
#ifdef CONFIG_IS_RECEIVER
    #define LORA_IS_RECEIVER true
#else
    #define LORA_IS_RECEIVER false
#endif

#endif // CONFIG_H
