#ifndef TASK_H
#define TASK_H

#include <stddef.h>

typedef enum {
    JOB_DNS_REQUEST,
    JOB_WS_SEND,
    JOB_INV_TRANSMIT
} job_type_t;

typedef struct {
    job_type_t type;
    void *data;
    size_t size;
} job_t;

void job_worker_freertos_task(void *arg);

#endif // TASK_H
