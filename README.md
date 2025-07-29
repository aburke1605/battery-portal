# Documentation

## ESP32
Each battery module is interfaced, through a BMS, to an ESP32 microcontroller.
The microcontroller is programmed in `C` with the ESP-IDF framework which is provided and maintained by [Espressif](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/index.html).
The primary aim of each ESP32 is to enable the monitoring and control of each battery module.
In practice, the microcontroller interacts with the BMS rather than directly with the module.
Data is exchanged between the ESP32 and the BMS, but also between the ESP32 and our online web server and / or other ESP32s via Wi-Fi communication.
In scenarios where the module is not within range of a network connection, Wi-Fi communication is replaced by radio transmission.

The diagram below illustrates the components of each individual battery module, as well as all possible forms of information exchange.

![information_exchange.png](img/information_exchange.png)

### BMS and LoRa
Shown in the zoomed bubble is a breakdown of one battery module unit. The battery itself is wired to a battery management system (BMS). At this point we are working with a four-series-cell lithium-ion battery and a [BQ40Z50-R1](https://www.ti.com/product/BQ40Z50) BMS. The BMS is then managed by the ESP32 through I<sup>2</sup>C communication. The ESP32 can simultaneously communicate with the radio LoRa transceiver through a separate SPI connection. The ESP32 comes pre-loaded with Wi-Fi and Bluetooth capabilities.

#### BMS


#### LoRA


## Webserver backend
<placeholder>

## UI frontend
<placeholder>
