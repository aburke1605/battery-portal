idf_component_register(SRCS "battery-portal.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_wifi esp_http_server nvs_flash driver spiffs)

spiffs_create_partition_image(storage ../images FLASH_IN_PROJECT)