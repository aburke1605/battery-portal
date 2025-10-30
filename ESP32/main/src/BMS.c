#include "BMS.h"

#include "I2C.h"
#include "TASK.h"
#include "config.h"
#include "global.h"
#include "utils.h"

#include "cJSON.h"
#include "esp_log.h"

static const char *TAG = "BMS";

static TimerHandle_t read_data_timer;

void reset() {
  uint8_t word[2] = {0};
  convert_uint_to_n_bytes(BMS_RESET_CMD, word, sizeof(word), true);

  write_word(I2C_MANUFACTURER_ACCESS, word, sizeof(word));

  // delay to allow reset to complete
  vTaskDelay(pdMS_TO_TICKS(I2C_DELAY));

  if (get_sealed_status() == 0 && true)
    ESP_LOGI(TAG, "Reset command sent successfully.");
}

void seal() {
  uint8_t word[2] = {0};
  convert_uint_to_n_bytes(BMS_SEAL_CMD, word, sizeof(word), true);
  write_word(I2C_MANUFACTURER_ACCESS, word, sizeof(word));

  // delay to allow seal to complete
  vTaskDelay(pdMS_TO_TICKS(I2C_DELAY));

  if (get_sealed_status() == 0)
    ESP_LOGI(TAG, "Seal command sent successfully.");
}

void unseal() {
  uint8_t word[2];

  convert_uint_to_n_bytes(BMS_UNSEAL_CMD_2, word, sizeof(word), true);
  write_word(I2C_MANUFACTURER_ACCESS, word, sizeof(word));

  convert_uint_to_n_bytes(BMS_UNSEAL_CMD_1, word, sizeof(word), true);
  write_word(I2C_MANUFACTURER_ACCESS, word, sizeof(word));

  // delay to allow unseal to complete
  vTaskDelay(pdMS_TO_TICKS(I2C_DELAY));

  if (get_sealed_status() == 1)
    ESP_LOGI(TAG, "Unseal command sent successfully.");
}

void full_access() {
  // first do regular unseal
  unseal();

  uint8_t word[2];

  convert_uint_to_n_bytes(BMS_FULL_ACCESS_CMD_2, word, sizeof(word), true);
  write_word(I2C_MANUFACTURER_ACCESS, word, sizeof(word));

  convert_uint_to_n_bytes(BMS_FULL_ACCESS_CMD_1, word, sizeof(word), true);
  write_word(I2C_MANUFACTURER_ACCESS, word, sizeof(word));

  // delay to allow full access to complete
  vTaskDelay(pdMS_TO_TICKS(I2C_DELAY));

  if (get_sealed_status() == 2)
    ESP_LOGI(TAG, "Full access command sent successfully.");
}

int8_t get_sealed_status() {
  // read OperationStatus
  uint8_t addr[2] = {0};
  convert_uint_to_n_bytes(I2C_OPERATION_STATUS_ADDR, addr, sizeof(addr), true);
  uint8_t data[4] = {0}; // expect 4 bytes for OperationStatus
  read_data_flash(addr, sizeof(addr), data, sizeof(data));
  bool SEC0 = 0b00000001 & data[1];
  bool SEC1 = 0b00000010 & data[1];

  if (SEC1 & SEC0)
    return 0; // sealed
  else if (SEC1 & !SEC0)
    return 1; // unsealed
  else if (!SEC1 & SEC0)
    return 2; // unsealed full access
  else
    return -1; // error
}

void update_telemetry_data() {
  uint8_t data_SBS[2] = {0};
  uint8_t address[2] = {0};
  uint8_t data_flash[2] = {0};
  uint8_t block_data_flash[32] = {0};

  // read sensor data
  read_SBS_data(I2C_RELATIVE_STATE_OF_CHARGE_ADDR, data_SBS, 1);
  telemetry_data.Q = (uint8_t)data_SBS[0];

  read_SBS_data(I2C_STATE_OF_HEALTH_ADDR, data_SBS, 1);
  telemetry_data.H = (uint8_t)data_SBS[0];

  read_SBS_data(I2C_TEMPERATURE_ADDR, data_SBS, 2);
  telemetry_data.aT = (uint16_t)(data_SBS[1] << 8 | data_SBS[0]);

  read_SBS_data(I2C_VOLTAGE_ADDR, data_SBS, 2);
  telemetry_data.V = (uint16_t)(data_SBS[1] << 8 | data_SBS[0]);

  read_SBS_data(I2C_CURRENT_ADDR, data_SBS, 2);
  telemetry_data.I = (int16_t)(data_SBS[1] << 8 | data_SBS[0]);

  convert_uint_to_n_bytes(I2C_DA_STATUS_1_ADDR, address, sizeof(address), true);
  read_data_flash(address, sizeof(address), block_data_flash,
                  sizeof(block_data_flash));
  telemetry_data.V1 =
      (uint16_t)(block_data_flash[1] << 8 | block_data_flash[0]);
  telemetry_data.V2 =
      (uint16_t)(block_data_flash[3] << 8 | block_data_flash[2]);
  telemetry_data.V3 =
      (uint16_t)(block_data_flash[5] << 8 | block_data_flash[4]);
  telemetry_data.V4 =
      (uint16_t)(block_data_flash[7] << 8 | block_data_flash[6]);
  telemetry_data.I1 =
      (int16_t)(block_data_flash[13] << 8 | block_data_flash[12]);
  telemetry_data.I2 =
      (int16_t)(block_data_flash[15] << 8 | block_data_flash[14]);
  telemetry_data.I3 =
      (int16_t)(block_data_flash[17] << 8 | block_data_flash[16]);
  telemetry_data.I4 =
      (int16_t)(block_data_flash[19] << 8 | block_data_flash[18]);

  convert_uint_to_n_bytes(I2C_DA_STATUS_2_ADDR, address, sizeof(address), true);
  read_data_flash(address, sizeof(address), block_data_flash,
                  sizeof(block_data_flash));
  telemetry_data.T1 =
      (uint16_t)(block_data_flash[3] << 8 | block_data_flash[2]);
  telemetry_data.T2 =
      (uint16_t)(block_data_flash[5] << 8 | block_data_flash[4]);
  telemetry_data.T3 =
      (uint16_t)(block_data_flash[7] << 8 | block_data_flash[6]);
  telemetry_data.T4 =
      (uint16_t)(block_data_flash[9] << 8 | block_data_flash[8]);
  telemetry_data.cT =
      (uint16_t)(block_data_flash[11] << 8 | block_data_flash[10]);

  // configurable data too
  convert_uint_to_n_bytes(I2C_OTC_THRESHOLD_ADDR, address, sizeof(address),
                          true);
  read_data_flash(address, sizeof(address), data_flash, sizeof(data_flash));
  telemetry_data.OTC = (int16_t)(data_flash[1] << 8 | data_flash[0]);
}

void read_data_callback(TimerHandle_t xTimer) {
  job_t job = {.type = JOB_UPDATE_DATA};

  if (xQueueSend(job_queue, &job, 0) != pdPASS)
    if (VERBOSE)
      ESP_LOGW(TAG, "Queue full, dropping job");
}

void start_read_data_timed_task() {
  read_data_timer = xTimerCreate("read_data_timer", pdMS_TO_TICKS(5000), pdTRUE,
                                 NULL, read_data_callback);
  assert(read_data_timer);
  xTimerStart(read_data_timer, 0);
}
