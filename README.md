# Remote Monitoring and Optimisation of Energy Storage Systems

## ESP32
Each battery module is interfaced, through a BMS, to an ESP32 microcontroller.
The microcontroller is programmed in `C` with the ESP-IDF framework which is provided and maintained by [Espressif](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/index.html).
The primary aim of each ESP32 is to enable the monitoring and control of each battery module.
In practice, the microcontroller interacts with the BMS rather than directly with the module.
Data is exchanged between the ESP32 and the BMS, but also between the ESP32 and our online web server and / or other ESP32s via Wi-Fi communication.
In scenarios where the module is not within range of a network connection, Wi-Fi communication is replaced by radio transmission.

The diagram below illustrates the components of each individual battery module, as well as all possible forms of information exchange.

![information_exchange.png](img/information_exchange.png)


### BMS, LoRa and GPS
Shown in the zoomed bubble is a breakdown of one battery module unit.
The battery itself is wired to a battery management system (BMS).
At this point we are working with a four-series-cell lithium-ion battery and a [BQ40Z50-R1](https://www.ti.com/product/BQ40Z50) BMS.
The BMS is then managed by the ESP32 through SMBus communication.
The ESP32 can also simultaneously communicate with the radio LoRa transceiver through a separate SPI connection.
The ESP32 comes pre-loaded with Wi-Fi and Bluetooth capabilities.


#### BMS
Communication with the BMS is achieved using the I<sup>2</sup>C driver, provided by ESP-IDF.
Defined from this are four core functions:
  * `read_SBS_data` and `write_word`: These read and write small quantites of data (usually one or two bytes) to and from specific addresses within the BMS.
  * `read_data_flash` and `write_data_flash`: These deal with larger exchanges of information stored in the Data Flash on the BMS (up to 32 bytes). The specific address is first transmitted to the BMS to select the information in question, before a read or write command is executed at the Data Flash address.

Building on these are BMS-specific functions designed to make use of the BMS capabilities.
These include the `seal`, `unseal` and `full_access` functions as well as `reset` which alter the state of the BMS.
Battery and individual cell data are obtained from the BMS within the `get_data` function.
This function is called regularly as the ESP32 transmits telemetry data to its WebSocket (WS) clients and/or the web server (see section on <b>Networking</b>).


#### LoRA
Communication with the LoRa transceiver is achieved using the SPI driver, provided by ESP-IDF.
Defined from this are two core functions:
  * `lora_read_register` and `lora_write_register`: These read and write singular bytes to and from specific addresses within the LoRa transceiver.

Building on these are functions designed to initialise the LoRa transceiver (see `lora_init` and `lora_configure_defaults`), and transmit/receive radio packets (see `lora_task`, `transmit` and `receive`).


#### GPS
<placeholder>



### Networking
This package makes use of the Wi-Fi capabilities of the ESP32 in multiple ways.
These are detailed below, in order of their chronological implementation in the project.

#### Local HTTP Server
The best way to display to users the telemetry data obtained from the BMS is via HTTP pages that can be served to any smartphone or other web-capable device.
This can be achieved by configuring the ESP32 to act as a Wi-Fi access point (AP) to which devices can connect, while simultaneously running a HTTP server.
By adding a DNS server, devices are automatically directed to the IP address of the ESP32 http server, mimicking the captive portals you find when connecting to public Wi-Fi hotspots.
From here HTTP clients are upgraded to WS clients, following a security check, such that the telemetry data is live-updated rather than once per page refresh.

The AP is configured by calling the `wifi_init` function.
Here the Wi-Fi mode is set to '`APSTA`', a combination of AP and station (STA) modes, to allow the ESP32 to simultaneously connect to other networks while devices connect to it.
At this point it is worth noting that while the ESP32 has its own WS clients, it can also register as a WS client of the web server when connected via Wi-Fi.
The name of the AP depends on various factors including the existence of other ESP32 APs with similar names, so the function first performs a scan for nearby APs.

Following this, the HTTP server is created in the `start_webserver` function, close to the default configuration provided by ESP-IDF.
The main purpose of the function is to register URI endpoints to one of the custom handler functions such that the server can respond to incoming requests at that endpoint.
Most URIs make use of the `file_serve_handler` function which can serve the most common types of files, including `.html`, `.css`, `.js`, `.png` and `.jpeg` files.
These external files are flashed to the ESP32s [SPIFFS](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/storage/spiffs.html) file system and mounted right after each boot, where they can later be found at their respective endpoints as requested.
There are also additional special use handler functions.
These include;
`login_handler`, which parses json log-in requests after a device connects to the AP, before either generating a new authentication token in memory or checking if a returning users token already exists;
`client_handler`, which registers new WS clients should they attempt a connection with an authentication token matching one of those in memory, before listening for incoming WS messages from existing clients, typically containing requests to change BMS settings;
and other simpler handlers to trigger various functions on the ESP32 by sending a HTTP request to the appropriate endpoint.

At this point, the set-up of the local HTTP server has been completed.
What remains is to register [FreeRTOS](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos.html) tasks, or functions, which run continuously on the ESP32.
The first of these is `dns_server_task`, which simply responds to all DNS requests made on the HTTP server with the IP address of the ESP32.
Next is `message_queue_task`, which is again relatively simple.
At various points throughout the code, WS messages to be sent from the ESP32 to the web server are added to a queue by calling the `send_message` function.
The job of `message_queue_task` is therefore to dequeue and either send or drop the message, depending on whether or not the ESP32 is connected to the internet (STA).
Lastly, `websocket_task` performs the main operations as the ESP32 runs.
It established a WS connection with the web server if the ESP32 is connected to the internet and creates and sends messages, most of the time containing telemetry data, to the server and to each of its own WS clients.
To handle incoming WS messages from the web server, another function named `websocket_event_handler` is registered as the ESP32 establishes a WS connection with the web server.
As already mentioned, incoming WS messages from the ESP32s own WS clients are processed similarly in `client_handler`.


## Webserver backend
<placeholder>

## UI frontend
<placeholder>
