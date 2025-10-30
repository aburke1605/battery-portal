set(SRCS
    "ESP32.c"
    "src/AP.c"
    "src/BMS.c"
    "src/DNS.c"
    "src/GPS.c"
    "src/I2C.c"
    "src/LoRa.c"
    "src/MESH.c"
    "src/SLAVE.c"
    "src/SPI.c"
    "src/TASK.c"
    "src/WS.c"
    "src/utils.c"
)

set(INCLUDE_DIRS "include")

if("${IDF_TARGET}" STREQUAL "linux")
    set(SUFFIX "_stub")
else()
    set(SUFFIX "")
endif()
set(REQUIRES
    esp_driver_gpio${SUFFIX}
    esp_driver_i2c${SUFFIX}
    esp_driver_spi${SUFFIX}
    esp_driver_uart${SUFFIX}
    esp_event
    esp_http_client
    esp_http_server
    esp_hw_support
    esp_netif${SUFFIX}
    esp_system
    esp_timer${SUFFIX}
    esp_websocket_client${SUFFIX}
    esp_wifi${SUFFIX}
    freertos
    json
    lwip${SUFFIX}
    nvs_flash
    spiffs${SUFFIX}
)

idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    REQUIRES ${REQUIRES}
)


if(NOT "${IDF_TARGET}" STREQUAL "linux")
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
