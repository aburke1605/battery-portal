if(NOT "${IDF_TARGET}" STREQUAL "linux")
    idf_component_register(SRCS "battery-portal.c" "src/AP.c" "src/I2C.c"
                           INCLUDE_DIRS "."
                           REQUIRES esp_netif esp_event esp_wifi esp_driver_i2c esp_hw_support)
else()
    idf_component_register(SRCS "battery-portal.c" "src/AP.c" "src/I2C.c"
                           INCLUDE_DIRS "."
                           REQUIRES esp_netif_stub esp_event esp_wifi_stub esp_driver_i2c_stub esp_hw_support)
endif()