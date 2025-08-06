idf_component_register(SRCS "battery-portal.c" "src/AP.c" "src/I2C.c" "src/BMS.c" "src/GPS.c" "src/DNS.c" "src/WS.c" "src/MESH.c" "src/LoRa.c" "src/utils.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_wifi esp_http_server esp_http_client nvs_flash driver spiffs json esp_driver_gpio wpa_supplicant)


# flash only needed stuff to SPIFFS
set(SPIFFS_BUILD_DIR ${CMAKE_BINARY_DIR}/spiffs_filtered)
execute_process(COMMAND rm -rf ${SPIFFS_BUILD_DIR})
execute_process(COMMAND mkdir -p ${SPIFFS_BUILD_DIR})
execute_process(
    COMMAND rsync -av
    --include=esp_login.html
    --exclude=*
    ${CMAKE_SOURCE_DIR}/website/templates/ ${SPIFFS_BUILD_DIR}/
)
execute_process(
    COMMAND rsync -av
    --include=esp32.html
    --include=favicon.png
    --include=assets/
    --include=assets/esp.js
    --include=assets/zap.js
    --include=assets/mock-socket.js
    --include=assets/zap.css
    --include=assets/mock-socket.css
    --exclude=assets/*
    --exclude=*
    ${CMAKE_SOURCE_DIR}/website/frontend/dist/ ${SPIFFS_BUILD_DIR}/
)

spiffs_create_partition_image(static ${SPIFFS_BUILD_DIR} FLASH_IN_PROJECT)
