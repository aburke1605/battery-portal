if(NOT "${IDF_TARGET}" STREQUAL "linux")
    idf_component_register(SRCS "battery-portal.c" "src/AP.c"
                           INCLUDE_DIRS "."
                           REQUIRES esp_netif esp_event esp_wifi)
else()
    idf_component_register(SRCS "battery-portal.c" "src/AP.c"
                           INCLUDE_DIRS "."
                           REQUIRES esp_netif_stub esp_event esp_wifi_stub)
endif()