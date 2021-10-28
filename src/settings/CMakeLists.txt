add_library(klogg_settings STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/persistentinfo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/configuration.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shortcuts.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/styles.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/persistable.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/persistentinfo.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/configuration.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/styles.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/shortcuts.h
)

target_include_directories(klogg_settings PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(klogg_settings PUBLIC 
    project_options
    project_warnings
    klogg_utils
    
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
)

if (KLOGG_USE_LTO)
set_property(TARGET klogg_settings PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()