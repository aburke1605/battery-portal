if(NOT "${IDF_TARGET}" STREQUAL "linux")
    return()
endif()

idf_component_register(SRCS "src/wifi_init.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_event)
