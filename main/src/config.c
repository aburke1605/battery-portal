#include <nvs_flash.h>
#include <nvs.h>
#include <esp_log.h>

#include "include/config.h"


// ESP-IDF limits NVS key names to 15 characters (excluding the null-terminator \0)
const char *KEY_AP_LOGIN_USERNAME = "username";
const char *KEY_AP_LOGIN_PASSWORD = "password";

const char *KEY_AP_WIFI_SSID = "ap_ssid";
const char *KEY_AP_WIFI_PASS = "ap_pass";

static const char *TAG = "GLOBAL_CONFIG";

void nvs_config_init(void) {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    // Initialize config
    // Should come from remote server TODO
    // AP login 
    save_global_config(KEY_AP_LOGIN_USERNAME, "admin");
    save_global_config(KEY_AP_LOGIN_PASSWORD, "123");
    // AP wifi
    save_global_config(KEY_AP_WIFI_SSID, "AceOn battery");
    save_global_config(KEY_AP_WIFI_PASS, "password");
    // 
}

void save_global_config(const char *key, const char *value) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("global_config", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS!");
        return;
    }
    err = nvs_set_str(nvs_handle, key, value);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Config [%s] saved as [%s]", key, value);
        nvs_commit(nvs_handle);
    } else {
        ESP_LOGE(TAG, "Failed to save config. Error: %s", esp_err_to_name(err));
    }
    nvs_close(nvs_handle);
}

void read_global_config(const char *key, char *value, size_t max_len) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("global_config", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS!");
        return;
    }
    size_t len = max_len;
    err = nvs_get_str(nvs_handle, key, value, &len);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Config [%s] read as [%s]", key, value);
    } else {
        ESP_LOGW(TAG, "No config found for key [%s]", key);
    }
    nvs_close(nvs_handle);
}