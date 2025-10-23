/*********
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Complete project details at https://RandomNerdTutorials.com/esp32-i2c-master-slave-arduino/
  ESP32 I2C Slave example: https://github.com/espressif/arduino-esp32/blob/master/libraries/Wire/examples/WireSlave/WireSlave.ino
*********/

#include "Wire.h"

#define I2C_DEV_ADDR 0x28
/*
  The SDA should be wired to GPIO 21 and the SCL to GPIO 22
*/

uint32_t i = 0;

void onRequest() {
  Wire.print(i++);
  Wire.print(" Packets.");
  Serial.println("onRequest");
  Serial.println();
}

void onReceive(int len) {
  if (len != 3) return; // need 3 bytes

  uint8_t buf[3];
  for (int i = 0; i < len && i < 3; i++) buf[i] = Wire.read();

  // little-endian
  uint8_t soc = buf[0];
  int16_t current = buf[1] | (buf[2] << 8);

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
  Wire.onRequest(onRequest);
  Wire.begin((uint8_t)I2C_DEV_ADDR);

/*#if CONFIG_IDF_TARGET_ESP32
  char message[64];
  snprintf(message, 64, "%lu Packets.", i++);
  Wire.slaveWrite((uint8_t *)message, strlen(message));
  Serial.print('Printing config %lu', i);
#endif*/
}

void loop() {
  
}