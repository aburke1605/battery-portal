#include "include/INV.h"

#include "include/config.h"
#include "include/I2C.h"

void get_display_data(uint8_t* data) {
    uint8_t data_SBS[2] = {0};

    // Q
    read_SBS_data(I2C_RELATIVE_STATE_OF_CHARGE_ADDR, data_SBS, sizeof(data_SBS));
    data[0] = data_SBS[1];
    data[1] = data_SBS[0];

    // I
    read_SBS_data(I2C_CURRENT_ADDR, data_SBS, sizeof(data_SBS));
    data[2] = data_SBS[1];
    data[3] = data_SBS[0];
}