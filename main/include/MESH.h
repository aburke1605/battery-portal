#ifndef MESH_H
#define MESH_H

#include "include/config.h"

void connect_to_root_task(void *pvParameters);

void mesh_websocket_task(void *pvParameters);

#endif // MESH_H
