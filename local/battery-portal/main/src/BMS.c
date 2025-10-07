#include "include/BMS.h"

#include "include/config.h"
#include "include/global.h"
#include "include/utils.h"

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
    /*

    write_word(I2C_MANUFACTURER_ACCESS, word, sizeof(word));

    // delay to allow reset to complete
    vTaskDelay(pdMS_TO_TICKS(I2C_DELAY));

    if (get_sealed_status() == 0 && true)
        ESP_LOGI(TAG, "Reset command sent successfully.");

    // resume
    if (suspending) vTaskResume(websocket_task_handle);
    */
}
