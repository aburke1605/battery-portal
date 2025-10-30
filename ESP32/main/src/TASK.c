#include "TASK.h"

#include "BMS.h"
#include "DNS.h"
#include "GPS.h"
#include "I2C.h"
#include "LoRa.h"
#include "MESH.h"
#include "WS.h"
#include "global.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"

static const char* TAG = "TASK";

void job_worker_freertos_task(void *arg) {
    job_t job;
    unsigned int n_jobs_remaining = 0;
    bool received = false;
    char job_type[32];
    int64_t start_time = 0;
    int64_t end_time = 0;

    while (true) {
        if (xQueueReceive(job_queue, &job, portMAX_DELAY) == pdPASS) {
            n_jobs_remaining = uxQueueMessagesWaiting(job_queue);
            received = true;
            start_time = esp_timer_get_time();
            switch (job.type) {
                case JOB_DNS_REQUEST:
                    snprintf(job_type, sizeof(job_type), "JOB_DNS_REQUEST");
                    handle_dns_request(job.data);
                    break;

                case JOB_UPDATE_DATA:
                    char bms[5] = "";
                    char gps[5] = "";
                    if (READ_BMS_ENABLED) {
                        update_telemetry_data();
                        strcpy(bms, " BMS");
                    }
                    if (READ_GPS_ENABLED) {
                        update_gps();
                        strcpy(gps, " GPS");
                    }
                    snprintf(job_type, sizeof(job_type), "JOB_UPDATE_DATA:%s%s", bms, gps);
                    break;

                case JOB_SLAVE_ESP32_TRANSMIT:
                    snprintf(job_type, sizeof(job_type), "JOB_SLAVE_ESP32_TRANSMIT");
                    write_to_slave_esp32();
                    break;

                case JOB_WS_SEND:
                    snprintf(job_type, sizeof(job_type), "JOB_WS_SEND");
                    send_websocket_data();
                    break;

                case JOB_WS_RECEIVE:
                    snprintf(job_type, sizeof(job_type), "JOB_WS_RECEIVE");
                    process_event(job.data);
                    break;

                case JOB_MESH_CONNECT:
                    snprintf(job_type, sizeof(job_type), "JOB_MESH_CONNECT");
                    connect_to_root();
                    break;

                case JOB_MESH_WS_SEND:
                    snprintf(job_type, sizeof(job_type), "JOB_MESH_WS_SEND");
                    send_mesh_websocket_data();
                    break;

                case JOB_MESH_MERGE:
                    snprintf(job_type, sizeof(job_type), "JOB_MESH_MERGE");
                    merge_root();
                    break;

                case JOB_LORA_RECEIVE:
                    snprintf(job_type, sizeof(job_type), "JOB_LORA_RECEIVE");
                    receive();
                    break;

                case JOB_LORA_TRANSMIT:
                    snprintf(job_type, sizeof(job_type), "JOB_LORA_TRANSMIT");
                    transmit();
                    break;

                default:
                    break;
            }
            end_time = esp_timer_get_time();

            // free heap-allocated job data if needed
            if (job.data) free(job.data);

            if (VERBOSE) {
                ESP_LOGI(TAG, "JOB WORKER: Number of jobs in queue:");
                ESP_LOGI(TAG, "  before: %d", received ? n_jobs_remaining + 1 : n_jobs_remaining);
                ESP_LOGI(TAG, "  after:  %d", n_jobs_remaining);
                if (received)
                    ESP_LOGI(TAG, "Processed \'%s\' in %f s", job_type, (float)(end_time-start_time)/1000000.0);
            }

            received = false;
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
