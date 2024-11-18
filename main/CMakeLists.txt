idf_component_register(SRCS "battery-portal.c" "src/AP.c" "src/I2C.c" "src/DNS.c" "src/WS.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_wifi esp_http_server nvs_flash driver spiffs json)

spiffs_create_partition_image(storage ../images FLASH_IN_PROJECT)