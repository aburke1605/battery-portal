if(NOT "${IDF_TARGET}" STREQUAL "linux")
    idf_component_register(SRCS "LINUX.c" "src/AP.c" "src/BMS.c" "src/DNS.c" "src/GPS.c" "src/I2C.c" "src/LoRa.c" "src/MESH.c" "src/SPI.c" "src/WS.c" "src/utils.c"
                           INCLUDE_DIRS "."
                           REQUIRES esp_netif esp_event esp_wifi esp_driver_i2c esp_hw_support nvs_flash spiffs freertos esp_driver_uart esp_http_server json esp_system lwip esp_websocket_client esp_http_client esp_driver_spi esp_driver_gpio esp_timer)
else()
    idf_component_register(SRCS "LINUX.c" "src/AP.c" "src/BMS.c" "src/DNS.c" "src/GPS.c" "src/I2C.c" "src/LoRa.c" "src/MESH.c" "src/SPI.c" "src/WS.c" "src/utils.c"
                           INCLUDE_DIRS "."
                           REQUIRES esp_netif_stub esp_event esp_wifi_stub esp_driver_i2c_stub esp_hw_support nvs_flash spiffs_stub freertos esp_driver_uart_stub esp_http_server json esp_system lwip_stub esp_websocket_client_stub esp_http_client esp_driver_spi_stub esp_driver_gpio_stub esp_timer_stub)
endif()