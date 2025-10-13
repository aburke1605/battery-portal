The `master` and `slave` examples show how two DOIT ESP32 DevKit V1 microcontrollers can talk to each other over I2C.

For the project, Ben needs regular telemetry data from the BMS so that his ESP32 can display this on the battery unit.

The main ESP32 therefore acts as the master and provides this, while Ben's ESP32 acts as the slave on the receiving end.

All that needs done is therefore to translate the `master` example to ESP-IDF so that it can run in parallel with the other code on the main ESP32.

For testing, we can run the `slave` example in the Arduino IDE as we don't need a working ESP-IDF version.
In fact, ESP-IDF is yet to provide I2C slave functionality on ESP32s (it currently exists only on ESP32-S3/C3/H2 modules, apparently).
