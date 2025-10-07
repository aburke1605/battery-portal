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

void seal() {
    // pause all tasks with regular I2C communication
    bool suspending = false;
    if (websocket_task_handle && eTaskGetState(websocket_task_handle) != eSuspended) {
        suspending = true;
        vTaskSuspend(websocket_task_handle);
    }

    uint8_t word[2] = {0};
    convert_uint_to_n_bytes(BMS_SEAL_CMD, word, sizeof(word), true);
    write_word(I2C_MANUFACTURER_ACCESS, word, sizeof(word));

    // delay to allow seal to complete
    vTaskDelay(pdMS_TO_TICKS(I2C_DELAY));

    if (get_sealed_status() == 0)
        ESP_LOGI(TAG, "Seal command sent successfully.");
    
    // resume
    if (suspending) vTaskResume(websocket_task_handle);
}

void unseal() {
    // pause all tasks with regular I2C communication
    bool suspending = false;
    if (websocket_task_handle && eTaskGetState(websocket_task_handle) != eSuspended) {
        suspending = true;
        vTaskSuspend(websocket_task_handle);
    }

    uint8_t word[2];

    convert_uint_to_n_bytes(BMS_UNSEAL_CMD_2, word, sizeof(word), true);
    write_word(I2C_MANUFACTURER_ACCESS, word, sizeof(word));

    convert_uint_to_n_bytes(BMS_UNSEAL_CMD_1, word, sizeof(word), true);
    write_word(I2C_MANUFACTURER_ACCESS, word, sizeof(word));

    // delay to allow unseal to complete
    vTaskDelay(pdMS_TO_TICKS(I2C_DELAY));

    if (get_sealed_status() == 1)
        ESP_LOGI(TAG, "Unseal command sent successfully.");

    // resume
    if (suspending) vTaskResume(websocket_task_handle);
}

void full_access() {
    // first do regular unseal
    unseal();

    // pause all tasks with regular I2C communication
    bool suspending = false;
    if (websocket_task_handle && eTaskGetState(websocket_task_handle) != eSuspended) {
        suspending = true;
        vTaskSuspend(websocket_task_handle);
    }

    uint8_t word[2];

    convert_uint_to_n_bytes(BMS_FULL_ACCESS_CMD_2, word, sizeof(word), true);
    write_word(I2C_MANUFACTURER_ACCESS, word, sizeof(word));

    convert_uint_to_n_bytes(BMS_FULL_ACCESS_CMD_1, word, sizeof(word), true);
    write_word(I2C_MANUFACTURER_ACCESS, word, sizeof(word));

    // delay to allow full access to complete
    vTaskDelay(pdMS_TO_TICKS(I2C_DELAY));

    if (get_sealed_status() == 2)
        ESP_LOGI(TAG, "Full access command sent successfully.");

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
