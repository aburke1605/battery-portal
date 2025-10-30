#pragma once

#include "esp_err.h"
#include "esp_log.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  const char *base_path;
  const char *partition_label;
  size_t max_files;
  bool format_if_mount_failed;
} esp_vfs_spiffs_conf_t;

static inline esp_err_t
esp_vfs_spiffs_register(const esp_vfs_spiffs_conf_t *conf) {
  ESP_LOGI("[spiffs_stub]", "esp_vfs_spiffs_register called");
  return ESP_OK;
}

#ifdef __cplusplus
}
#endif
