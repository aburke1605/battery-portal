if(NOT "${IDF_TARGET}" STREQUAL "linux")
    idf_component_register(SRCS "battery-portal.c" "src/AP.c"
                           INCLUDE_DIRS "."
                           REQUIRES esp_netif)
else()
    idf_component_register(SRCS "battery-portal.c" "src/AP.c"
                           INCLUDE_DIRS "."
                           REQUIRES esp_netif_stub)
endif()