#include "TASK.h"

#include "DNS.h"
#include "global.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"

static const char* TAG = "TASK";

void job_worker_task(void *arg) {
    job_t job;

    while (true) {
        if (xQueueReceive(job_queue, &job, portMAX_DELAY) == pdTRUE) {
            switch (job.type) {
                case JOB_DNS_REQUEST:
                    handle_dns_request(job.data);
                    break;

                default:
                    break;
            }

            // free heap-allocated job data if needed
            if (job.data) free(job.data);
        }
    }
}
