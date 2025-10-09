#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    I2C_NUM_0 = 0,
} i2c_port_t;

typedef int i2c_port_num_t;

typedef struct i2c_master_bus_t *i2c_master_bus_handle_t;
typedef struct i2c_master_dev_t *i2c_master_dev_handle_t;

#ifdef __cplusplus
}
#endif
