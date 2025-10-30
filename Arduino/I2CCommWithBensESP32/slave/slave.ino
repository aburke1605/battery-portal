#include "Wire.h"

#define I2C_DEV_ADDR 0x28
// The SDA should be wired to GPIO 21 and the SCL to GPIO 22

void onReceive(int len) {
  if (len != 3) return; // expect 3 bytes

  uint8_t buf[3];
  for (int i = 0; i < len && i < 3; i++) buf[i] = Wire.read();

  uint8_t soc = buf[0];
  int16_t current = buf[1] | (buf[2] << 8); // little-endian

  Serial.printf("onReceive[%d]: ", len);
  for (int i = 0; i < len; i++) Serial.printf("0x%02X ", buf[i]);
  Serial.println();

  Serial.printf("State of Charge: %u %%\n", soc);
  Serial.printf("Current: %d mA\n", current);
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Wire.onReceive(onReceive);
  Wire.begin((uint8_t)I2C_DEV_ADDR);
}

void loop() {

}
