add_library(klogg_versioncheck STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/versionchecker.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/versionchecker.cpp
)

set_target_properties(klogg_versioncheck PROPERTIES AUTOMOC ON)

target_include_directories(klogg_versioncheck PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(klogg_versioncheck PUBLIC 
        project_options
        project_warnings

        klogg_settings 
        klogg_utils

        Qt${QT_VERSION_MAJOR}::Network
)
