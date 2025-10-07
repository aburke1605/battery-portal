#include "include/utils.h"

#include "include/config.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_spiffs.h"

void initialise_nvs() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    nvs_handle_t nvs;
    esp_err_t err = nvs_open("WIFI", NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        ESP_LOGE("NVS", "Couldn't open NVS namespace");
        return;
    }

    size_t len;
    err = nvs_get_str(nvs, "SSID", NULL, &len);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        // not yet initialised - this must be first boot since the most recent flash
        nvs_set_str(nvs, "SSID", WIFI_SSID);
        nvs_set_str(nvs, "PASSWORD", WIFI_PASSWORD);
        nvs_set_u8(nvs, "AUTO_CONNECT", WIFI_AUTO_CONNECT?1:0);
        nvs_commit(nvs);
    }
    nvs_close(nvs);
}

void initialise_spiffs() {
    esp_vfs_spiffs_conf_t config_static = {
        .base_path = "/static",
        .partition_label = "static",
        .max_files = 5,
        .format_if_mount_failed = true
    };
    esp_err_t ret = esp_vfs_spiffs_register(&config_static);

    if (ret != ESP_OK) {
        ESP_LOGE("utils", "Failed to initialise SPIFFS (%s)", esp_err_to_name(ret));
        return;
    }
}

void convert_uint_to_n_bytes(unsigned int input, uint8_t *output, size_t n_bytes, bool little_endian) {
    for(size_t i=0; i<n_bytes; i++)
        output[i] = (input >> ((little_endian?n_bytes-1-i:i)*8)) & 0xFF;
}
