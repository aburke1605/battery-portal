#include "I2C.h"

#include "config.h"
#include "utils.h"
#include "INV.h"

#include <stdbool.h>

#include "driver/i2c_master.h"
#include "driver/i2c_types.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

static i2c_master_bus_handle_t bms_bus = NULL;
static i2c_master_dev_handle_t bms_device = NULL;

static i2c_master_bus_handle_t inv_bus = NULL;
static i2c_master_dev_handle_t inv_device = NULL;

static const char* TAG = "I2C";

esp_err_t i2c_master_init(void) {
    // BMS bus
    i2c_master_bus_config_t bms_bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_NUM,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 0,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t err = i2c_new_master_bus(&bms_bus_cfg, &bms_bus);
    if (err != ESP_OK) return err;

    i2c_device_config_t bms_cfg = {
        .device_address = I2C_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ
    };
    err |= i2c_master_bus_add_device(bms_bus, &bms_cfg, &bms_device);


    // INV bus
    i2c_master_bus_config_t inv_bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_NUM,
        .scl_io_num = 8,
        .sda_io_num = 9,
        .glitch_ignore_cnt = 0,
        .flags.enable_internal_pullup = true,
    };

    err = i2c_new_master_bus(&inv_bus_cfg, &bms_bus);
    if (err != ESP_OK) return err;

    i2c_device_config_t inv_cfg = {
        .device_address = UNIT_I2C_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ
    };
    err |= i2c_master_bus_add_device(inv_bus, &inv_cfg, &inv_device);

    return err;
}

esp_err_t check_device() {
    esp_err_t ret = i2c_master_probe(bms_bus, I2C_ADDR, I2C_MASTER_TIMEOUT_MS);
    if (ret != ESP_OK) ESP_LOGE(TAG, "I2C device not found at address 0x%x.", I2C_ADDR);

    return ret;
}

void device_scan(void) {
    ESP_LOGI(TAG, "Scanning for devices...");
    uint8_t n_devices = 0;
    for (uint8_t i = 1; i < 127; i++) {
        esp_err_t ret = i2c_master_probe(bms_bus, i, pdMS_TO_TICKS(1000));

        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "I2C device found at address 0x%02X", i);
            n_devices++;
        }
    }

    if (n_devices == 0) ESP_LOGW(TAG, "No devices found.");
}

esp_err_t read_SBS_data(uint8_t reg, uint8_t* data, size_t data_size) {
    esp_err_t ret = check_device();
    if (ret != ESP_OK) return ret;

    if (!data || data_size == 0) {
        ESP_LOGE(TAG, "Invalid data buffer or length.");
        return ESP_ERR_INVALID_ARG;
    }

    // transmit the register address
    ret = i2c_master_transmit(bms_device, &reg, 1, I2C_MASTER_TIMEOUT_MS);
    if (ret != ESP_OK) return ret;

    // receive the response data
    ret = i2c_master_receive(bms_device, data, data_size, I2C_MASTER_TIMEOUT_MS);

    return ret;
}

void write_word(uint8_t command, uint8_t* word, size_t word_size) {
    esp_err_t ret = check_device();
    if (ret != ESP_OK) return;

    uint8_t data[1 + word_size];
    data[0] = command;
    for (size_t i=0; i<word_size; i++)
        data[1 + i] = word[word_size - 1 - i]; // assumed little-endian(?)

    ret = i2c_master_transmit(bms_device, data, sizeof(data), I2C_MASTER_TIMEOUT_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write word!");
        return;
    }
}

void read_data_flash(uint8_t* address, size_t address_size, uint8_t* data, size_t data_size) {
    esp_err_t ret = check_device();
    if (ret != ESP_OK) return;

    uint8_t addr[2 + address_size];
    addr[0] = I2C_MANUFACTURER_BLOCK_ACCESS;
    addr[1] = address_size;
    for (int i=0; i<address_size; i++)
        addr[2 + i] = address[address_size - 1 - i]; // little-endian

    // write the number of bytes of the address of the data we want to read, and the address itself
    ret = i2c_master_transmit(bms_device, addr, sizeof(addr), I2C_MASTER_TIMEOUT_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write in read_data_flash!");
        return;
    }
    // initiate read
    uint8_t MAC = I2C_MANUFACTURER_BLOCK_ACCESS;
    ret = i2c_master_transmit(bms_device, &MAC, 1, I2C_MASTER_TIMEOUT_MS);


    uint8_t buff[1 + address_size + 32]; // each ManufacturerBlockAccess() block is maximum 32 bytes,
                                         // plus first byte which states the number of bytes in returned block,
                                         // plus extra bytes to echo the address
    for (size_t i=0; i<sizeof(buff); i++) buff[i] = 0; // initialise to zeros

    ret = i2c_master_receive(bms_device, buff, sizeof(buff), I2C_MASTER_TIMEOUT_MS);
    if (ret == ESP_OK) {
        for (size_t i=0; i<MIN(data_size, sizeof(buff)-1-address_size); i++)
            data[i] = buff[1 + address_size + i];
    } else {
        ESP_LOGE(TAG, "Failed to read in read_data_flash!");
    }
}

void write_data_flash(uint8_t* address, size_t address_size, uint8_t* data, size_t data_size) {
    esp_err_t ret = check_device();
    if (ret != ESP_OK) return;

    uint8_t block[1 + 1 + address_size + data_size];
    block[0] = I2C_MANUFACTURER_BLOCK_ACCESS;
    block[1] = address_size + data_size;

    for (uint8_t i=0; i<address_size; i++)
        block[2 + i] = address[address_size - 1 - i]; // little-endian

    for (uint8_t i=0; i<data_size; i++)
        block[2 + address_size + i] = data[i];

    ret = i2c_master_transmit(bms_device, block, sizeof(block), I2C_MASTER_TIMEOUT_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write block!");
    }
    vTaskDelay(pdMS_TO_TICKS(100));
}

void write_to_unit() {
    uint8_t data[3] = {0};
    get_display_data(data);

    esp_err_t ret = i2c_master_transmit(inv_device, data, sizeof(data), I2C_MASTER_TIMEOUT_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write to Ben's ESP32!");
        return;
    }
}

void read_from_unit() {
    uint8_t data[16] = {0};
    i2c_master_receive(inv_device, data, sizeof(data), I2C_MASTER_TIMEOUT_MS);
    printf("requestFrom: %zu\n", sizeof(data));
    for (uint8_t i=0; i<sizeof(data); i++)
        printf("0x%x ", data[i]);
    printf("\n");
    for (uint8_t i=0; i<sizeof(data); i++)
        if (data[i] != 0xff) printf("%c", data[i]);
    printf("\n");

    return;
}
