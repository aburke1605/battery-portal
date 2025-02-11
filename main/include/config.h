// config.h
#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>

extern const char *KEY_AP_LOGIN_USERNAME;
extern const char *KEY_AP_LOGIN_PASSWORD;

void nvs_init(void);
void save_global_config(const char *key, const char *value);
void read_global_config(const char *key, char *value, size_t max_len);

#endif // GLOBAL_CONFIG_H
