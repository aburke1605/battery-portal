# Remote Monitoring and Optimisation of Energy Storage Systems

## ESP32
Each battery module is interfaced, through a battery management system (BMS), to an ESP32 microcontroller.
The microcontroller is programmed in `C` with the ESP-IDF framework which is provided and maintained by [Espressif](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/index.html).
The primary aim of each ESP32 is to enable the monitoring and control of each battery module.
In practice, the microcontroller interacts with the BMS rather than directly with the module.
Data is exchanged between the ESP32 and the BMS, but also between the ESP32 and our online web server and / or other ESP32s via Wi-Fi communication.
In scenarios where the module is not within range of a network connection, Wi-Fi communication is replaced by radio transmission.

The diagram below illustrates the components of each individual battery module, as well as all possible forms of information exchange.

![information_exchange.png](img/information_exchange.png)


### BMS, LoRa and GPS
Shown in the zoomed bubble is a breakdown of one battery module unit.
The battery itself is wired to a BMS.
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
  * `spi_read_register` and `spi_write_register`: These read and write singular bytes to and from specific addresses within the LoRa transceiver.

Building on these are functions designed to initialise the LoRa transceiver (see `spi_init` and `lora_init`), and transmit/receive radio packets (see `lora_task`, `transmit` and `receive`).


#### GPS
<placeholder>



### Wi-Fi Networking
This package makes use of the Wi-Fi capabilities of the ESP32 in multiple ways.
These are detailed below.

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

#### WebSocket Management
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

#### MESH Network
For reasons discussed later (see section on <b>Radio Communication</b>), it is useful to form a local Wi-Fi network of battery units which are within communication range of each other.
This network is referred to as a 'MESH', with the following logic:
  * Each MESH consists of one and only one 'ROOT', to which the other ESP32s (known as nodes) make WS connections to.
    A string `"ROOT"` is prepended to the SSID of the ROOT ESP32 AP to signify to other ESP32s its existence.
    A scan for any AP containing the string in its SSID is defined in a function named `wifi_scan`, which is called within the already discussed AP set-up `wifi_init` function.
    If a ROOT is found, the string is omitted and the ESP32 behaves as a node.
  * The logic of connecting nodes to a ROOT is defined in a FreeRTOS function named `connect_to_root_task`.
    It consists of an initial ROOT scan, which should be positive given that the ESP32 has booted as a node, and connection attempt.
    If the connection fails, another ROOT scan is performed to check if the ROOT still exists, this repeating until a successful connection is made.
    If the ROOT is instead not found, the node delays for a random period* of at least five seconds before performing one last scan.
    If the ROOT is still not found, the node restarts so that it will become a ROOT on its next boot (assuming the set-up scan still does not find another ROOT).
  * In a scenario where two separate MESHs are brought together, the two ROOT ESP32s deterministically nominate one as the sole ROOT moving forward while the other restarts.
    In this way, the rebooted ROOT and each of its previous nodes will be forced to scan and connect to the persisting ROOT, forming a singular MESH.
    The code which acheives this is defined in the `merge_root_task` FreeRTOS function.
    Since both ROOTs have the same default IP address, one of the two is changed (to `192.168.10.1`, ROOT B) while the other remains unchanged (`192.168.4.1`, ROOT A) to allow them to communicate with one another when connected.
    This happens simultaneously on both ROOTs by comparing MAC addresses, which are unique to each, such that each deterministically knows whether or not to change IP address.
    Next, ROOT B sends a HTTP request to an API endpoint registered on the local server of ROOT A (`http://192.168.4.1/api_num_clients`), which returns its current number of nodes.
    This is compared with its own number and that with fewer restarts.
    If ROOT B has fewer, the restart is simple.
    If ROOT A has fewer, ROOT B sends a HTTP request to another API endpoint (`http://192.168.4.1/no_you_restart`) to instruct it to restart instead.
  * If a MESHs ROOT unexpectedly dies, the remaining nodes will naturally nominate a new ROOT given the logic already described in `connect_to_root_task`.
  * Lastly, the `mesh_websocket_task` FreeRTOS function does a very similar job to `websocket_task`: forming and sending telemetry WS messages from nodes to ROOT, again using `websocket_event_handler` to handle incoming WS messages from the ROOT.

    <h6>*The reason for random periods is such that each node in a given MESH delays for a different length of time. Therefore, one node will restart earlier than the others and become the ROOT, which the others will then detect and connect to.</h6>



### Radio Communication
When an ESP32 is not connected to the internet, it can still communicate with the web server via radio transmission.
To achieve this, a dedicated ESP32 known as the 'HUB' must be permanently situated somewhere it can receive and transmit radio messages whilst maintaining an internet connection to communicate with the web server.
As alluded to in the previous section, MESH networks of battery units that are without internet connection are the ESP32s on the other end of radio communication with the HUB.
Specifically, since all nodes in a MESH already share their telemetry data with the ROOT, radio communication is solely between the HUB and ROOTs.
There are two core radio functions, `receive` and `transmit`, whose functionalities differ depending on whether the ESP32 is configured as the HUB or a ROOT.

The `receive` function waits for an incoming radio transmission at the configured frequency (868 MHz), where a bit flip on the LoRa transceiver module is detected on the ESP32 through SPI communication.
The number of bytes in the radio message may be longer than the maximum number per payload, hence the function allows for message 'chunks' to be received consecutively and will combine these to reconstruct the original message.
On the HUB, a received message will originate from a ROOT transmission containing telemetry data.
The HUB therefore simply forwards the received message to the web server via its WS connection, using `send_message`.
On ROOTs, received messages originate from the HUB and will contain requests made by users of the web portal online.
The ROOT therefore parses the request, determining which member of the MESH it is for (if any), then either executing the request if it is for the ROOT itself or forwarding it to the correct node via WS message.

The logic of the `transmit` function follows suit.
If the HUB receives a WS message from the web server, this is forwarded to all in-range ROOTs via radio transmission.
The design of the online portal (see sections on <b>Web Server Backend</b> and <b>UI Frontend</b>) ensures that when a user sends a request to a given battery module, this goes either directly to the ESP32 if it is connected to the internet, or to the HUB where it is then forwarded by radio if not.
The HUB itself is not visible or mentioned on the portal.
ROOTs instead transmit the telemetry data of their own battery unit as well as that of each node in its MESH which it has received via WS messages.
As in the `receive` function, the `transmit` function deals with long messages by dividing them into chunks and transmitting consecutively.

Both `receive` and `transmit` are called within a FreeRTOS function named `lora_task`.
Since there are duty cycle laws in most countries around the fair usage of public radio frequencies, time delays sepearate consecutive radio transmissions, the length of which a function of the number of bytes in the transmitted message.
Calling `receive` is therefore the default action of `lora_task`.
Only once the previous delay has been served is `transmit` called again.




## Web Server Backend
<placeholder>

## UI Frontend
<placeholder>
