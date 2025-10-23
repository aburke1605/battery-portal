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
    data[0] = telemetry_data.Q;

    // I
    data[1] =  telemetry_data.I       & 0xff;
    data[2] = (telemetry_data.I >> 8) & 0xff;
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
