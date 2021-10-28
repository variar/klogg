add_library(klogg_version STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/klogg_version.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/klogg_version.cpp
)

target_include_directories(klogg_version PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(klogg_version PUBLIC 
    project_options
    project_warnings
    Qt${QT_VERSION_MAJOR}::Core
)

add_dependencies(klogg_version generate_version)
