#include <esp_log.h>
#include <driver/i2c.h>

esp_err_t i2c_master_init(void);

void device_scan(void);

uint16_t read_2byte_data(int REG_ADDR);
