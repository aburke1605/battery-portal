#include "include/BMS.h"

#include "include/I2C.h"
#include "include/config.h"
#include "include/global.h"
#include "include/utils.h"

#include "esp_log.h"

static const char* TAG = "BMS";

void reset() {
    // pause all tasks with regular I2C communication
    bool suspending = false;
    if (websocket_task_handle && eTaskGetState(websocket_task_handle) != eSuspended) {
        suspending = true;
        vTaskSuspend(websocket_task_handle);
    }

    uint8_t word[2] = {0};
    convert_uint_to_n_bytes(BMS_RESET_CMD, word, sizeof(word), true);

    write_word(I2C_MANUFACTURER_ACCESS, word, sizeof(word));

    // delay to allow reset to complete
    vTaskDelay(pdMS_TO_TICKS(I2C_DELAY));

    if (get_sealed_status() == 0 && true)
        ESP_LOGI(TAG, "Reset command sent successfully.");

    // resume
    if (suspending) vTaskResume(websocket_task_handle);
}

int8_t get_sealed_status() {
    // read OperationStatus
    uint8_t addr[2] = {0};
    convert_uint_to_n_bytes(I2C_OPERATION_STATUS_ADDR, addr, sizeof(addr), true);
    uint8_t data[4] = {0}; // expect 4 bytes for OperationStatus
    read_data_flash(addr, sizeof(addr), data, sizeof(data));
    bool SEC0 = 0b00000001 & data[1];
    bool SEC1 = 0b00000010 & data[1];

    if (SEC1 & SEC0)
        return 0; // sealed
    else if (SEC1 & !SEC0)
        return 1; // unsealed
    else if (!SEC1 & SEC0)
        return 2; // unsealed full access
    else
        return -1; // error
}
