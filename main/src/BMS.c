#include "include/BMS.h"

#include "include/config.h"
#include "include/global.h"
#include "include/I2C.h"
#include "include/utils.h"

#include <esp_log.h>
#include <cJSON.h>

static const char* TAG = "BMS";

void reset() {
    // pause all tasks with regular I2C communication
    bool suspending = false;
    if (websocket_task_handle && eTaskGetState(websocket_task_handle) != eSuspended) {
        suspending = true;
        vTaskSuspend(websocket_task_handle);
    }

    uint8_t word[2] = { (I2C_CONTROL_RESET_SUBCMD >> 8) & 0xFF, I2C_CONTROL_RESET_SUBCMD & 0xFF };
    write_word(0x00, word, sizeof(word));

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

    uint8_t word[2] = { 0x00, 0x30 };
    write_word(0x00, word, sizeof(word));

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

    word[0] = (I2C_CONTROL_UNSEAL_SUBCMD_2 >> 8) & 0xFF;
    word[1] = I2C_CONTROL_UNSEAL_SUBCMD_2 & 0xFF;
    write_word(0x00, word, sizeof(word));

    word[0] = (I2C_CONTROL_UNSEAL_SUBCMD_1 >> 8) & 0xFF;
    word[1] = I2C_CONTROL_UNSEAL_SUBCMD_1 & 0xFF;
    write_word(0x00, word, sizeof(word));

    if (get_sealed_status() == 1)
        ESP_LOGI(TAG, "Unseal command sent successfully.");

    // resume
    if (suspending) vTaskResume(websocket_task_handle);
}

char* get_data(bool local) {
    // create JSON object with sensor data
    cJSON *data = cJSON_CreateObject();

    uint8_t two_bytes[2];

    // read sensor data
    // these values are big-endian
    read_bytes(0, I2C_STATE_OF_CHARGE_REG, two_bytes, sizeof(two_bytes));
    cJSON_AddNumberToObject(data, "Q", two_bytes[1] << 8 | two_bytes[0]);

    read_bytes(0, I2C_STATE_OF_HEALTH_REG, two_bytes, sizeof(two_bytes));
    cJSON_AddNumberToObject(data, "H", two_bytes[1] << 8 | two_bytes[0]);

    read_bytes(0, I2C_VOLTAGE_REG, two_bytes, sizeof(two_bytes));
    cJSON_AddNumberToObject(data, "V", round_to_dp((float)(two_bytes[1] << 8 | two_bytes[0]) / 1000.0, 1));

    read_bytes(0, I2C_CELL1_VOLTAGE_REG, two_bytes, sizeof(two_bytes));
    cJSON_AddNumberToObject(data, "V1", round_to_dp((float)((int16_t)(two_bytes[1] << 8 | two_bytes[0])) / 1000.0, 1));

    read_bytes(0, I2C_CELL2_VOLTAGE_REG, two_bytes, sizeof(two_bytes));
    cJSON_AddNumberToObject(data, "V2", round_to_dp((float)((int16_t)(two_bytes[1] << 8 | two_bytes[0])) / 1000.0, 1));

    read_bytes(0, I2C_CELL3_VOLTAGE_REG, two_bytes, sizeof(two_bytes));
    cJSON_AddNumberToObject(data, "V3", round_to_dp((float)((int16_t)(two_bytes[1] << 8 | two_bytes[0])) / 1000.0, 1));

    read_bytes(0, I2C_CELL4_VOLTAGE_REG, two_bytes, sizeof(two_bytes));
    cJSON_AddNumberToObject(data, "V4", round_to_dp((float)((int16_t)(two_bytes[1] << 8 | two_bytes[0])) / 1000.0, 1));

    read_bytes(0, I2C_CURRENT_REG, two_bytes, sizeof(two_bytes));
    cJSON_AddNumberToObject(data, "I", round_to_dp((float)((int16_t)(two_bytes[1] << 8 | two_bytes[0])) / 1000.0, 1));

    read_bytes(0, I2C_CELL1_CURRENT_REG, two_bytes, sizeof(two_bytes));
    cJSON_AddNumberToObject(data, "I1", round_to_dp((float)((int16_t)(two_bytes[1] << 8 | two_bytes[0])) / 1000.0, 1));

    read_bytes(0, I2C_CELL2_CURRENT_REG, two_bytes, sizeof(two_bytes));
    cJSON_AddNumberToObject(data, "I2", round_to_dp((float)((int16_t)(two_bytes[1] << 8 | two_bytes[0])) / 1000.0, 1));

    read_bytes(0, I2C_CELL3_CURRENT_REG, two_bytes, sizeof(two_bytes));
    cJSON_AddNumberToObject(data, "I3", round_to_dp((float)((int16_t)(two_bytes[1] << 8 | two_bytes[0])) / 1000.0, 1));

    read_bytes(0, I2C_CELL4_CURRENT_REG, two_bytes, sizeof(two_bytes));
    cJSON_AddNumberToObject(data, "I4", round_to_dp((float)((int16_t)(two_bytes[1] << 8 | two_bytes[0])) / 1000.0, 1));

    read_bytes(0, I2C_TEMPERATURE_REG, two_bytes, sizeof(two_bytes));
    cJSON_AddNumberToObject(data, "aT", round_to_dp((float)(two_bytes[1] << 8 | two_bytes[0]) / 10.0 - 273.15, 1));

    read_bytes(0, I2C_INT_TEMPERATURE_REG, two_bytes, sizeof(two_bytes));
    cJSON_AddNumberToObject(data, "iT", round_to_dp((float)(two_bytes[1] << 8 | two_bytes[0]) / 10.0 - 273.15, 1));

    read_bytes(0, I2C_CELL1_TEMPERATURE_REG, two_bytes, sizeof(two_bytes));
    cJSON_AddNumberToObject(data, "T1", round_to_dp((float)(two_bytes[1] << 8 | two_bytes[0]) / 10.0 - 273.15, 1));

    read_bytes(0, I2C_CELL2_TEMPERATURE_REG, two_bytes, sizeof(two_bytes));
    cJSON_AddNumberToObject(data, "T2", round_to_dp((float)(two_bytes[1] << 8 | two_bytes[0]) / 10.0 - 273.15, 1));

    read_bytes(0, I2C_CELL3_TEMPERATURE_REG, two_bytes, sizeof(two_bytes));
    cJSON_AddNumberToObject(data, "T3", round_to_dp((float)(two_bytes[1] << 8 | two_bytes[0]) / 10.0 - 273.15, 1));

    read_bytes(0, I2C_CELL4_TEMPERATURE_REG, two_bytes, sizeof(two_bytes));
    cJSON_AddNumberToObject(data, "T4", round_to_dp((float)(two_bytes[1] << 8 | two_bytes[0]) / 10.0 - 273.15, 1));


    // configurable data too
    // these values are little-endian
    read_bytes(I2C_DISCHARGE_SUBCLASS_ID, I2C_BL_OFFSET, two_bytes, sizeof(two_bytes));
    cJSON_AddNumberToObject(data, "BL", two_bytes[0] << 8 | two_bytes[1]);

    read_bytes(I2C_DISCHARGE_SUBCLASS_ID, I2C_BH_OFFSET, two_bytes, sizeof(two_bytes));
    cJSON_AddNumberToObject(data, "BH", two_bytes[0] << 8 | two_bytes[1]);

    read_bytes(I2C_CURRENT_THRESHOLDS_SUBCLASS_ID, I2C_CHG_CURRENT_THRESHOLD_OFFSET, two_bytes, sizeof(two_bytes));
    cJSON_AddNumberToObject(data, "CCT", two_bytes[0] << 8 | two_bytes[1]);

    read_bytes(I2C_CURRENT_THRESHOLDS_SUBCLASS_ID, I2C_DSG_CURRENT_THRESHOLD_OFFSET, two_bytes, sizeof(two_bytes));
    cJSON_AddNumberToObject(data, "DCT", two_bytes[0] << 8 | two_bytes[1]);

    read_bytes(I2C_CHARGE_INHIBIT_CFG_SUBCLASS_ID, I2C_CHG_INHIBIT_TEMP_LOW_OFFSET, two_bytes, sizeof(two_bytes));
    cJSON_AddNumberToObject(data, "CITL", two_bytes[0] << 8 | two_bytes[1]);

    read_bytes(I2C_CHARGE_INHIBIT_CFG_SUBCLASS_ID, I2C_CHG_INHIBIT_TEMP_HIGH_OFFSET, two_bytes, sizeof(two_bytes));
    cJSON_AddNumberToObject(data, "CITH", two_bytes[0] << 8 | two_bytes[1]);

    if (local) {
        // only needed for local clients
        cJSON_AddBoolToObject(data, "connected_to_WiFi", connected_to_WiFi);
        cJSON *full_data = cJSON_CreateObject();
        cJSON_AddItemToObject(full_data, ESP_ID, data);

        char *full_data_string = cJSON_PrintUnformatted(full_data);
        cJSON_Delete(full_data);

        return full_data_string;
    }

    char *data_string = cJSON_PrintUnformatted(data);
    cJSON_Delete(data);

    return data_string;
}
