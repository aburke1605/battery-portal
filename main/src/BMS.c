#include "include/BMS.h"

#include "include/config.h"
#include "include/global.h"
#include "include/I2C.h"

#include <esp_log.h>

static const char* TAG = "BMS";

esp_err_t reset() {
    esp_err_t ret;

    // pause all tasks with regular I2C communication
    bool suspending = false;
    if (websocket_task_handle && eTaskGetState(websocket_task_handle) != eSuspended) {
        suspending = true;
        vTaskSuspend(websocket_task_handle);
    }

    // send reset subcommand
    ret = write_data(I2C_CONTROL_REG, I2C_CONTROL_RESET_SUBCMD, 2);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send reset command.");
        return ret;
    }

    // delay to allow reset to complete
    vTaskDelay(pdMS_TO_TICKS(I2C_DELAY));

    // unseal again
    unseal();

    ESP_LOGI(TAG, "Reset command sent successfully.");

    vTaskDelay(pdMS_TO_TICKS(I2C_DELAY));

    // resume
    if (suspending) vTaskResume(websocket_task_handle);

    return ESP_OK;
}

esp_err_t unseal() {
    esp_err_t ret;

    // pause all tasks with regular I2C communication
    bool suspending = false;
    if (websocket_task_handle && eTaskGetState(websocket_task_handle) != eSuspended) {
        suspending = true;
        vTaskSuspend(websocket_task_handle);
    }

    // reverse order
    ret = write_data(I2C_CONTROL_REG, I2C_CONTROL_UNSEAL_SUBCMD_2, 2);
    ret = write_data(I2C_CONTROL_REG, I2C_CONTROL_UNSEAL_SUBCMD_1, 2);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send unseal command.");
        return ret;
    }

    ESP_LOGI(TAG, "Unseal command sent successfully.");
    
    // resume
    if (suspending) vTaskResume(websocket_task_handle);

    return ESP_OK;
}
