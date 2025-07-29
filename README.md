# Documentation

## ESP32
Each battery module is interfaced, through a BMS, to an ESP32 microcontroller.
The microcontroller is programmed in C with the ESP-IDF framework which is provided and maintained by Espressif.
The primary aim of each ESP32 is to enable the monitoring and control of each battery module.
In practice, the microcontroller interacts with the BMS rather than directly with the module.
Data is exchanged between the ESP32 and the BMS, but also between the ESP32 and our online web server and / or other ESP32s via Wi-Fi communication.
In scenarios where the module is not within range of a network connection, Wi-Fi communication is replaced by radio transmission.

The diagram below details all possible forms of information exchange.

![information_exchange.png](img/information_exchange.png)

## Webserver backend
<placeholder>

## UI frontend
<placeholder>
