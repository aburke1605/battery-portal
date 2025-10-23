#include "SLAVE.h"

#include "I2C.h"
#include "TASK.h"
#include "config.h"
#include "global.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"

static const char* TAG = "SLAVE";

static TimerHandle_t slave_esp32_timer;

void get_display_data(uint8_t* data) {
    // Q
    data[0] = telemetry_data.Q;

    // I
    data[1] =  telemetry_data.I       & 0xff;
    data[2] = (telemetry_data.I >> 8) & 0xff;
}

void slave_esp32_callback(TimerHandle_t xTimer) {
    job_t job = {
        .type = JOB_SLAVE_ESP32_TRANSMIT
    };

    if (xQueueSend(job_queue, &job, 0) != pdPASS)
        if (VERBOSE) ESP_LOGW(TAG, "Queue full, dropping job");
}

void start_slave_esp32_timed_task() {
    slave_esp32_timer = xTimerCreate("slave_esp32_timer", pdMS_TO_TICKS(SLAVE_DELAY), pdTRUE, NULL, slave_esp32_callback);
    assert(slave_esp32_timer);
    xTimerStart(slave_esp32_timer, 0);
}
