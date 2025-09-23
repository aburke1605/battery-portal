#include "include/BMS.h"

#include "include/config.h"
#include "include/global.h"
#include "include/I2C.h"
#include "include/GPS.h"
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

char* get_data() {
    // create JSON object with sensor data
    cJSON *data = cJSON_CreateObject();

    GPRMC* gprmc = get_gps();
    if (gprmc) {
        cJSON_AddNumberToObject(data, "t", gprmc->time);
        cJSON_AddNumberToObject(data, "d", gprmc->date);
        cJSON_AddNumberToObject(data, "lat", gprmc->latitude);
        cJSON_AddNumberToObject(data, "lon", gprmc->longitude);
        free(gprmc);
    } else {
        cJSON_AddNumberToObject(data, "t", 0);
        cJSON_AddNumberToObject(data, "d", 0);
        cJSON_AddNumberToObject(data, "lat", 53.40680302139558);
        cJSON_AddNumberToObject(data, "lon", -2.968465812849439);
    }

    uint8_t data_SBS[2] = {0};
    uint8_t address[2] = {0};
    uint8_t data_flash[2] = {0};
    uint8_t block_data_flash[32] = {0};

    // read sensor data
    read_SBS_data(I2C_RELATIVE_STATE_OF_CHARGE_ADDR, data_SBS, sizeof(data_SBS));
    cJSON_AddNumberToObject(data, "Q", data_SBS[1] << 8 | data_SBS[0]);

    read_SBS_data(I2C_STATE_OF_HEALTH_ADDR, data_SBS, sizeof(data_SBS));
    cJSON_AddNumberToObject(data, "H", data_SBS[1] << 8 | data_SBS[0]);

    read_SBS_data(I2C_TEMPERATURE_ADDR, data_SBS, sizeof(data_SBS));
    cJSON_AddNumberToObject(data, "aT", round_to_dp((float)(data_SBS[1] << 8 | data_SBS[0]) / 10.0 - 273.15, 1));

    read_SBS_data(I2C_VOLTAGE_ADDR, data_SBS, sizeof(data_SBS));
    cJSON_AddNumberToObject(data, "V", round_to_dp((float)((int16_t)(data_SBS[1] << 8 | data_SBS[0])) / 1000.0, 1));

    read_SBS_data(I2C_CURRENT_ADDR, data_SBS, sizeof(data_SBS));
    cJSON_AddNumberToObject(data, "I", round_to_dp((float)((int16_t)(data_SBS[1] << 8 | data_SBS[0])) / 1000.0, 1));

    convert_uint_to_n_bytes(I2C_DA_STATUS_1_ADDR, address, sizeof(address), true);
    read_data_flash(address, sizeof(address), block_data_flash, sizeof(block_data_flash));
    cJSON_AddNumberToObject(data, "V1", round_to_dp((float)((int16_t)(block_data_flash[1] << 8 | block_data_flash[0])) / 1000.0, 2));
    cJSON_AddNumberToObject(data, "V2", round_to_dp((float)((int16_t)(block_data_flash[3] << 8 | block_data_flash[2])) / 1000.0, 2));
    cJSON_AddNumberToObject(data, "V3", round_to_dp((float)((int16_t)(block_data_flash[5] << 8 | block_data_flash[4])) / 1000.0, 2));
    cJSON_AddNumberToObject(data, "V4", round_to_dp((float)((int16_t)(block_data_flash[7] << 8 | block_data_flash[6])) / 1000.0, 2));
    cJSON_AddNumberToObject(data, "I1", round_to_dp((float)((int16_t)(block_data_flash[13] << 8 | block_data_flash[12])) / 1000.0, 2));
    cJSON_AddNumberToObject(data, "I2", round_to_dp((float)((int16_t)(block_data_flash[15] << 8 | block_data_flash[14])) / 1000.0, 2));
    cJSON_AddNumberToObject(data, "I3", round_to_dp((float)((int16_t)(block_data_flash[17] << 8 | block_data_flash[16])) / 1000.0, 2));
    cJSON_AddNumberToObject(data, "I4", round_to_dp((float)((int16_t)(block_data_flash[19] << 8 | block_data_flash[18])) / 1000.0, 2));

    convert_uint_to_n_bytes(I2C_DA_STATUS_2_ADDR, address, sizeof(address), true);
    read_data_flash(address, sizeof(address), block_data_flash, sizeof(block_data_flash));
    cJSON_AddNumberToObject(data, "T1", round_to_dp((float)(block_data_flash[3] << 8 | block_data_flash[2]) / 10.0 - 273.15, 2));
    cJSON_AddNumberToObject(data, "T2", round_to_dp((float)(block_data_flash[5] << 8 | block_data_flash[4]) / 10.0 - 273.15, 2));
    cJSON_AddNumberToObject(data, "T3", round_to_dp((float)(block_data_flash[7] << 8 | block_data_flash[6]) / 10.0 - 273.15, 2));
    cJSON_AddNumberToObject(data, "T4", round_to_dp((float)(block_data_flash[9] << 8 | block_data_flash[8]) / 10.0 - 273.15, 2));
    cJSON_AddNumberToObject(data, "cT", round_to_dp((float)(block_data_flash[11] << 8 | block_data_flash[10]) / 10.0 - 273.15, 1));


    // configurable data too
    convert_uint_to_n_bytes(I2C_OTC_THRESHOLD_ADDR, address, sizeof(address), true);
    read_data_flash(address, sizeof(address), data_flash, sizeof(data_flash));
    cJSON_AddNumberToObject(data, "OTC", round_to_dp((float)(data_flash[1] << 8 | data_flash[0]) / 10.0, 1));

    // wifi connection status
    cJSON_AddBoolToObject(data, "wifi", connected_to_WiFi);


    // construct full message
    cJSON *message = cJSON_CreateObject();

    char* esp_id = esp_id_string();
    cJSON_AddStringToObject(message, "esp_id", esp_id);
    free(esp_id);

    cJSON_AddStringToObject(message, "type", "data");

    cJSON_AddItemToObject(message, "content", data);

    char *data_string = cJSON_PrintUnformatted(message);
    cJSON_Delete(message);

    return data_string;
}
