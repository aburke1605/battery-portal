#include "TASK.h"

#include "DNS.h"
#include "I2C.h"
#include "LoRa.h"
#include "MESH.h"
#include "WS.h"
#include "global.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"

static const char* TAG = "TASK";

void job_worker_freertos_task(void *arg) {
    job_t job;

    while (true) {
        if (xQueueReceive(job_queue, &job, portMAX_DELAY) == pdPASS) {
            switch (job.type) {
                case JOB_DNS_REQUEST:
                    handle_dns_request(job.data);
                    break;

                case JOB_INV_TRANSMIT:
                    write_to_unit();
                    break;

                case JOB_WS_SEND:
                    send_websocket_data();
                    break;

                case JOB_MESH_CONNECT:
                    connect_to_root();
                    break;

                case JOB_MESH_WS_SEND:
                    send_mesh_websocket_data();
                    break;

                case JOB_MESH_MERGE:
                    merge_root();
                    break;

                case JOB_LORA_RECEIVE:
                    receive();
                    break;

                default:
                    break;
            }

            // free heap-allocated job data if needed
            if (job.data) free(job.data);
        }
    }
}
