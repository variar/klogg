add_library(klogg_logging STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/log.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/logger.h
)

target_include_directories(klogg_logging PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_definitions(klogg_logging PUBLIC "-DQT_MESSAGELOGCONTEXT")
target_link_libraries(klogg_logging PUBLIC 
    project_options
    project_warnings
    Qt${QT_VERSION_MAJOR}::Core
)

if (KLOGG_USE_LTO)
set_property(TARGET klogg_logging PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()