#include "include/utils.h"

#include "include/config.h"

#include "esp_log.h"
#include "nvs_flash.h"

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
