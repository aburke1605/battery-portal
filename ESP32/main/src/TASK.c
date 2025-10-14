#include "TASK.h"

#include "global.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>

static const char* TAG = "TASK";

void job_worker_task(void *arg) {

    job_t job;
    while (true) {
        if (xQueueReceive(job_queue, &job, portMAX_DELAY) == pdTRUE) {
            switch (job.type) {
                case JOB_TYPE_I2C_WRITE:
                    ESP_LOGI(TAG, "I2C write job received");
                    break;
                case JOB_TYPE_LOG:
                    ESP_LOGI(TAG, "Log job: %s", (char*)job.data);
                    break;
                case JOB_TYPE_OTHER:
                    ESP_LOGI(TAG, "Other job");
                    break;
            }

            // free heap-allocated job data if needed
            if (job.data) free(job.data);
        }
    }
}