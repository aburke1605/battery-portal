if(NOT "${IDF_TARGET}" STREQUAL "linux")
    idf_component_register(SRCS "ESP32.c" "src/AP.c" "src/BMS.c" "src/DNS.c" "src/GPS.c" "src/I2C.c" "src/LoRa.c" "src/MESH.c" "src/SPI.c" "src/WS.c" "src/utils.c"
                           INCLUDE_DIRS "."
                           REQUIRES esp_netif esp_event esp_wifi esp_driver_i2c esp_hw_support nvs_flash spiffs freertos esp_driver_uart esp_http_server json esp_system lwip esp_websocket_client esp_http_client esp_driver_spi esp_driver_gpio esp_timer)
else()
    idf_component_register(SRCS "ESP32.c" "src/AP.c" "src/BMS.c" "src/DNS.c" "src/GPS.c" "src/I2C.c" "src/LoRa.c" "src/MESH.c" "src/SPI.c" "src/WS.c" "src/utils.c"
                           INCLUDE_DIRS "."
                           REQUIRES esp_netif_stub esp_event esp_wifi_stub esp_driver_i2c_stub esp_hw_support nvs_flash spiffs_stub freertos esp_driver_uart_stub esp_http_server json esp_system lwip_stub esp_websocket_client_stub esp_http_client esp_driver_spi_stub esp_driver_gpio_stub esp_timer_stub)

    # flash only needed stuff to SPIFFS
    set(SPIFFS_BUILD_DIR ${CMAKE_BINARY_DIR}/spiffs_filtered)
    execute_process(COMMAND rm -rf ${SPIFFS_BUILD_DIR})
    execute_process(COMMAND mkdir -p ${SPIFFS_BUILD_DIR})
    execute_process(
        COMMAND rsync -av
        --include=esp32.html
        --include=favicon.png
        --include=assets/
        --include=assets/esp.js
        --include=assets/AuthRequire.js
        --include=assets/AuthRequire.css
        --exclude=assets/*
        --exclude=*
        ${CMAKE_SOURCE_DIR}/../website/frontend/dist/ ${SPIFFS_BUILD_DIR}/
    )

    spiffs_create_partition_image(static ${SPIFFS_BUILD_DIR} FLASH_IN_PROJECT)
endif()
