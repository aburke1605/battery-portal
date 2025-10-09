if(NOT "${IDF_TARGET}" STREQUAL "linux")
    return()
endif()

idf_component_register(INCLUDE_DIRS "include")
