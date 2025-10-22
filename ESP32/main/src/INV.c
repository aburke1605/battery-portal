#include "INV.h"

#include "I2C.h"
#include "TASK.h"
#include "config.h"
#include "global.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"

static const char* TAG = "INV";

static TimerHandle_t inverter_timer;

void get_display_data(uint8_t* data) {
    uint8_t data_SBS[2] = {0};

    // Q
    read_SBS_data(I2C_RELATIVE_STATE_OF_CHARGE_ADDR, data_SBS, sizeof(data_SBS));
    data[0] = data_SBS[0];
    data[1] = data_SBS[1];

    // I
    read_SBS_data(I2C_CURRENT_ADDR, data_SBS, sizeof(data_SBS));
    data[2] = data_SBS[0];
    data[3] = data_SBS[1];
}

void inverter_callback(TimerHandle_t xTimer) {
    job_t job = {
        .type = JOB_INV_TRANSMIT
    };

    if (xQueueSend(job_queue, &job, 0) != pdPASS)
        if (VERBOSE) ESP_LOGW(TAG, "Queue full, dropping job");
}

void start_inverter_timed_task() {
    inverter_timer = xTimerCreate("inverter_timer", pdMS_TO_TICKS(INV_DELAY), pdTRUE, NULL, inverter_callback);
    assert(inverter_timer);
    xTimerStart(inverter_timer, 0);
}
