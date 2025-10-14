#ifndef TASK_H
#define TASK_H

#include <stddef.h>

typedef enum {
    JOB_TYPE_I2C_WRITE,
    JOB_TYPE_LOG,
    JOB_TYPE_OTHER
} job_type_t;

typedef struct {
    job_type_t type;
    void *data;
    size_t length;
} job_t;

void job_worker_task(void *arg);

#endif // TASK_H
