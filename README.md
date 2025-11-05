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


---


### Task Scheduling and Job Processing
After initialisation, repeated executions on the ESP32 (such as reading telemetry data, sending messages to the web server, etc) are typically managed through [FreeRTOS](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos.html) tasks, or functions.
In this project there are around 10 individual taks required, each of which introducing extra overhead.
A limit on the number was found between 8-12 tasks (depending on the size of each) before the relatively small memory availability of the ESP32 is fully consumed.

To avoid wasting memory defining multiple tasks, the project instead defines a job queue and one job-worker task to process the jobs in the queue (see `job_worker_freertos_task`).
Jobs are added to the queue in one of three ways:
  * Software timed: The most regular method, a new job is added to the queue after a pre-defined time period elapses in the internal clock, e.g. regularly reading telemetry data.
  * Hardware ISR: A new job is queued when triggered by a hardware interrupt service routine (ISR), e.g. a new radio message is received.
  * External event: An event detected by the software adds a new job to the queue, e.g. an incoming message from the web server.

---


### BMS, LoRa and GPS
The battery itself is wired to a BMS.
At this point we are working with a four-series-cell lithium-ion battery and a [BQ40Z50-R1](https://www.ti.com/product/BQ40Z50) BMS.
The BMS is then managed by the ESP32 through SMBus communication.
The ESP32 can also simultaneously communicate with the long range (LoRa) radio transceiver through a separate SPI connection and with the GPS module through a UART connection.
The ESP32 itself comes pre-loaded with Wi-Fi and Bluetooth capabilities.


#### BMS
Communication with the BMS is achieved using the I<sup>2</sup>C driver, provided by ESP-IDF.
Defined from this are four core functions:
  * `read_SBS_data` and `write_word`: These read and write small quantites of data (usually one or two bytes) to and from specific addresses within the BMS.
  * `read_data_flash` and `write_data_flash`: These deal with larger exchanges of information stored in the Data Flash on the BMS (up to 32 bytes). The specific address is first transmitted to the BMS to select the information in question, before a read or write command is executed at the Data Flash address.

Building on these are BMS-specific functions designed to make use of the BMS capabilities.
These include the `seal`, `unseal` and `full_access` functions as well as `reset` which alter the state of the BMS.
Battery and individual cell data are obtained from the BMS within the software-timed `update_telemetry_data` task executable.
This function is called regularly as the ESP32 transmits telemetry data to its WebSocket (WS) clients and/or the web server (see section on <b>Networking</b>).


#### LoRA
Communication with the LoRa transceiver is achieved using the SPI driver, provided by ESP-IDF.
Defined from this are two core functions:
  * `spi_read_register` and `spi_write_register`: These read and write singular bytes to and from specific addresses within the LoRa transceiver.

Building on these are functions designed to initialise the LoRa transceiver (see `spi_init` and `lora_init`), and transmit/receive radio packets (see `transmit` and `receive`).


#### GPS
Communication with the GPS module is achieved using the UART driver, provided by ESP-IDF.
The module spits out NMEA sentences, one of which we are interested in: GPRMC.
The GPRMC sentence details the latitude and longitude as well as the current time and date which can be used to accurately timestamp the telemetry data.
Without this, the timestamp would be recorded as the time of arrival at the web server, which can be up to a minute after extraction from the BMS.
The `update_gps` function, which uses a core ESP-IDF function `uart_read_bytes`, is called to regularly pull GPS data from the module, before being appended to outgoing telemetry messages.


---


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
What remains is to register the job-queueing tasks which execute regularly on the ESP32.
The first of these is `dns_server_task`, which simply responds to all DNS requests made on the HTTP server with the IP address of the ESP32, and is in fact the only FreeRTOS task other than the main job-worker task.
Then, the software-timed `send_websocket_data` task executable performs the main operations as the ESP32 runs.
It established a WS connection with the web server if the ESP32 is connected to the internet and creates and sends messages, most of the time containing telemetry data, to the server and to each of its own WS clients.
To handle incoming WS messages from the web server, another function named `websocket_event_handler` is registered as the ESP32 establishes a WS connection with the web server, with a external-event task executable named `process_event` which is queued on each incoming event.
As already mentioned, incoming WS messages from the ESP32's own WS clients are processed similarly in `client_handler`.

#### MESH Network
For reasons discussed later (see section on <b>Radio Communication</b>), it is useful to form a local Wi-Fi network of battery units which are within communication range of each other.
This network is referred to as a 'MESH', with the following logic:
  * Each MESH consists of one and only one 'ROOT', to which the other ESP32s (known as nodes) make WS connections to.
    A string `"ROOT"` is prepended to the SSID of the ROOT ESP32 AP to signify to other ESP32s its existence.
    A scan for any AP containing the string in its SSID is defined in a function named `wifi_scan`, which is called within the already discussed AP set-up `wifi_init` function.
    If a ROOT is found, the string is omitted and the ESP32 behaves as a node.
  * The logic of connecting nodes to a ROOT is defined in a software-timed task with executable named `connect_to_root`.
    It consists of an initial ROOT scan, which should be positive given that the ESP32 has booted as a node, and connection attempt.
    If the connection fails, another ROOT scan is performed to check if the ROOT still exists, this repeating until a successful connection is made.
    If the ROOT is instead not found, the node delays for a random period* of at least five seconds before performing one last scan.
    If the ROOT is still not found, the node restarts so that it will become a ROOT on its next boot (assuming the set-up scan still does not find another ROOT).
  * In a scenario where two separate MESHs are brought together, the two ROOT ESP32s deterministically nominate one as the sole ROOT moving forward while the other restarts.
    In this way, the rebooted ROOT and each of its previous nodes will be forced to scan and connect to the persisting ROOT, forming a singular MESH.
    The code which acheives this is defined in the software-timed `merge_root` task executable.
    Since both ROOTs have the same default IP address, one of the two is changed (to `192.168.10.1`, ROOT B) while the other remains unchanged (`192.168.4.1`, ROOT A) to allow them to communicate with one another when connected.
    This happens simultaneously on both ROOTs by comparing MAC addresses, which are unique to each, such that each deterministically knows whether or not to change IP address.
    Next, ROOT B sends a HTTP request to an API endpoint registered on the local server of ROOT A (`http://192.168.4.1/api_num_clients`), which returns its current number of nodes.
    This is compared with its own number and that with fewer restarts.
    If ROOT B has fewer, the restart is simple.
    If ROOT A has fewer, ROOT B sends a HTTP request to another API endpoint (`http://192.168.4.1/no_you_restart`) to instruct it to restart instead.
  * If a MESHs ROOT unexpectedly dies, the remaining nodes will naturally nominate a new ROOT given the logic already described in `connect_to_root`.
  * Lastly, the software-timed `send_mesh_websocket_data` task executable does a very similar job to `send_websocket_data`: forming and sending telemetry WS messages from nodes to ROOT, again using `websocket_event_handler` to handle incoming WS messages from the ROOT.

    <h6>*The reason for random periods is such that each node in a given MESH delays for a different length of time. Therefore, one node will restart earlier than the others and become the ROOT, which the others will then detect and connect to.</h6>


---


### Radio Communication
When an ESP32 is not connected to the internet, it can still communicate with the web server via radio transmission.
To achieve this, a dedicated ESP32 known as the 'HUB' must be permanently situated somewhere it can receive and transmit radio messages whilst maintaining an internet connection to communicate with the web server.
As alluded to in the previous section, MESH networks of battery units that are without internet connection are the ESP32s on the other end of radio communication with the HUB.
Specifically, since all nodes in a MESH already share their telemetry data with the ROOT, radio communication is solely between the HUB and ROOTs.
There are two core radio functions, `receive` and `transmit`, whose functionalities differ depending on whether the ESP32 is configured as the HUB or a ROOT.

The `receive` function waits for an incoming radio transmission at the configured frequency (868 MHz), where a bit flip on the LoRa transceiver module is detected on the ESP32 through SPI communication.
The number of bytes in the radio message may be longer than the maximum number per payload, hence the function allows for message 'chunks' to be received consecutively and will combine these to reconstruct the original message.
On the HUB, a received message will originate from a ROOT transmission containing telemetry data.
The HUB therefore simply forwards the received message to the web server via its WS connection.
On ROOTs, received messages originate from the HUB and will contain requests made by users of the web portal online.
The ROOT therefore parses the request, determining which member of the MESH it is for (if any), then either executing the request if it is for the ROOT itself or forwarding it to the correct node via WS message.

The logic of the `transmit` function follows suit.
If the HUB receives a WS message from the web server, this is forwarded to all in-range ROOTs via radio transmission.
The design of the online portal (see sections on <b>Web Server Backend</b> and <b>UI Frontend</b>) ensures that when a user sends a request to a given battery module, this goes either directly to the ESP32 if it is connected to the internet, or to the HUB where it is then forwarded by radio if not.
The HUB itself is not visible or mentioned on the portal.
ROOTs transmit the telemetry data of their own battery unit as well as that of each node in its MESH which it has received via WS messages.
As in the `receive` function, the `transmit` function deals with long messages by dividing them into chunks and transmitting consecutively.

`transmit` is called within a software-timed task while `receive` is called within an ISR triggered task.
Since there are duty cycle laws in most countries around the fair usage of public radio frequencies, time delays sepearate consecutive radio transmissions, the length of which a function of the number of bytes in the transmitted message.
Therefore, only once the previous delay has elapsed is `transmit` called again.
To minimise this necessary delay between consecutive messages, the json format used to exchange WS messages between server and client is converted to and from custom-defined binary packets, using functions named `json_to_binary` and `binary_to_json`.
Different packets are defined for different types of message, e.g. telemetry data in `radio_data_packet` or a request in `radio_request_packet`.
A radio transmission containing $N$ individual messages therefore can not be trivially divided into $N$ equal binary packets, since packet types are generally of unequal length.
To remove this ambiguity between transmitter and receiver, the first byte of each kind of binary packet defines the packet `type`.
With knowledge of this, the receiver can deduce the number of bytes within the message that constitute the current packet, as well as the packet type.
As a fail-safe, the first byte of the message is set to the total number of packets contained within the message.
So that the receiver can identify chunked messages or discard background noise, each message is encoded as follows:
  * The message is 'framed' by adding `FRAME_END` (`0x7E`) bytes to the beginning and end.
  * Any naturally occuring `0x7E` bytes in the binary packet are 'escaped' by the `FRAME_ESC` (`0x7D`) and `ESC_END` (`0x5E`) bytes: `0x7E -> 0x7D 0x5E`
  * Any naturally occuring `0x7D` bytes in the binary packet are 'escaped' again by `FRAME_ESC` but with the `ESC_ESC` (`0x5D`) byte instead: `0x7D -> 0x7D 0x5D`

This logic and the reverse is implemented in `encode_frame` and `decode_frame` functions which are called in `transmit` and `receive`, respectively.

A simplified example encoded radio message is as follows:
```
byte | data
-----------
0    | 0x7E     - FRAME_END
1    | 0x02     -   total number of binary packets
2    | 0x00     -   beginning of first packet, telemetry data type: 0 == 0x00
3    | 0x01     -     ESP32 ID: 1 == 0x01
4    | 0x43     -     battery state of charge: 67% == 0x43
5    | 0x7D     -     \
6    | 0x5D     -      battery voltage: 32,000mV == 0x7D00 -> 0x7D5D00
7    | 0x00     -     /
8    | 0x02     -   beginning of second packet, request data type: 2 == 0x02
9    | 0x02     -     ESP32 ID: 2 == 0x02
10   | 0x03     -     request type: ENUM(reset BMS) -> 3 == 0x03
11   | 0x7E     - FRAME_END
```

---
---


## Web App
The web app and database are hosted with Microsoft Azure cloud services for consistent availability and scalability.
Deployment to Azure is managed with GitHub Actions (see `.github/workflows/main_batteryportal.yml` for details).


---


### Backend Server
The web app backend server is implemented using the [Flask](https://flask.palletsprojects.com) Python framework. The architecture is spread across four main components:
  1. `app.main` <br>
    Registers the sites core endpoints to functions which respond by serving the HTML files which are built (see section on <b>Frontend UI</b>) before starting the web app.

  2. `app.db` <br>
    Defines the two table structures which manage battery telemetry data in the project database.
    The first of these is named `battery_info`, which contains only columns needed for displaying the status of the network of battery modules.
    Individual tables named `battery_data_<esp_id>`, containing the actual telemetry data for each battery, are dynamically created / deleted as modules are in real life, while `battery_info` persists.
    Additionally, a number of API endpoints are registered to let the site query and update the database through the web app.

  3. `app.ws` <br>
    Registers endpoints through which clients can establish WebSocket connections with the server using [Flask-Sock](https://flask-sock.readthedocs.io).
    There are two kinds of defined client: ESP32 and browser (site users).
    Telemetry data is sent to the server by the ESP32 microcontrollers interfaced with each battery module via WebSocket rather than with simple HTTP POST.
    In this way the list of live WebSocket connections is used to indicate to site users which modules are currently online.
    The live telemetry data is then forwarded to active site users via WebSocket so that the most recent data is always displayed without needing a page refresh.
    The WebSockets are also used to forward any module configuration changes made by site users back through the connection to the corresponding ESP32.

  4. `app.user` <br>
    Manages user access to the web app, defined by three table structures.
    The first of these is named `roles` and defines unique user types, each with certain permissions / abilities.
    The second, `users`, stores actual user data (email, password, role, etc), while the final table is a simple association table linking the `users` and `roles` tables and is aptly named `roles_users`.
    [Flask-Security](https://flask-security-too.readthedocs.io) is used to implement user authentication, authorisation, role management, and password security.

#### Database
Telemetry (and user) data is stored in a MySQL database.
This is primarily to allow research of second-life batteries at a later stage, but is also useful for module history and visualisation on the web site.
Database schema management and versioning are handled with [Flask-Migrate](https://flask-migrate.readthedocs.io).


---


### Frontend UI
The web site frontend is built with TypeScript, React, Tailwind CSS, and [TailAdmin](https://tailadmin.com/) (free React version).
It consists of a simple public home page and the remote monitoring portal which is accessible only through validated user login.

#### Portal
The portal provides tabs for battery module remote management and monitoring features.
These include:
  * <b>Dashboard</b> <br>
    Displays a summary of the network and key metrics, including graphical respresentations of telemetry data.
  * <b>Batteries</b> <br>
    Lists each registered battery module, displaying their connection status to the server and any MESH network relationships.
    The list view can be toggled to a map view, displaying the most recent GPS coordinates of each module with a Google maps API.
  * <b>Users</b> <br>
    Here the administrator can create, remove or manage user access to the portal.
  * <b>Settings</b> <br>
    `YET TO BE IMPLEMENTED`
  * <b>Database</b> <br>
    Area to execute SQL query commands on the database tables through APIs.
    This exsists mainly for ease of administrator access to the database on Azure, whose IP is otherwise inaccessible without additional cloud virtual machine instances.

From the <b>Batteries</b> tab, users can click <b>Details</b> on any battery card to open a more detailed view of that module's most recent (live, if online) telemetry data, including a <b>Digital Twin</b> view for visualisation.
Configuration and optimisation of the module's BMS settings are customised here, though only successfully on modules which are currently online.

#### Local ESP32 HTTP Server
As mentioned already (see section on <b>ESP32 > Wi-Fi Networking</b>), telemetry data is also accessible by battery module users by connecting a smart phone or other browser-capable device to the HTTP server hosted locally on the ESP32 that is interfaced with the module.
The only necessary frontend to be served by each individual module is the same detailed view which is accessed by clicking <b>Details</b> on a module on the web site.
For this reason, a separate route is compiled specifically for ESP32 HTTP servers, producing an entry file named `esp32.html` which is flashed to each ESP32.
This is separate from the main entry files `home.html` and `index.html` which are served by the web app.
However, the functionality of the detailed view remains the same, and so the ESP32 HTTP server programming mimics that of the web app so that the frontend seamlessly interacts with both.


---



## Local Development and Testing

To enable end-to-end testing of the web app locally before deployment, a production-like instance of the app is containerised with Docker and served via an Nginx proxy.

The Docker configuration can be found in `Dockerfile` and `compose.yml`.
The web app image uses the Nginx config file in `nginx/battery-portal.conf`, however the Nginx proxy can also be run locally without Docker by executing the `nginx/local_setup.sh` script.

Otherwise, the containerised app is started with:
```bash
docker compose up
```

---
