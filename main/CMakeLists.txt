idf_component_register(SRCS "battery-portal.c" "src/AP.c" "src/I2C.c" "src/DNS.c" "src/ARP.c" "src/WS.c" "src/utils.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_wifi esp_http_server esp_http_client nvs_flash driver spiffs json esp_driver_gpio)

spiffs_create_partition_image(staticstorage ../website/static/portal FLASH_IN_PROJECT)
spiffs_create_partition_image(templatesstorage ../website/templates/portal FLASH_IN_PROJECT)