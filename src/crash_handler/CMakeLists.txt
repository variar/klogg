add_library(klogg_crash_handler STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/crashhandler.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/issuereporter.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/memory_info.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/cpu_info.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/issuereporter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/memory_info.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/cpu_info.cpp
)

set_target_properties(klogg_crash_handler PROPERTIES AUTOMOC ON)

target_include_directories(klogg_crash_handler PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(klogg_crash_handler PUBLIC 
    project_options
    project_warnings
    klogg_version
    klogg_utils
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
)

if(KLOGG_USE_SENTRY)
    target_compile_definitions(klogg_crash_handler PUBLIC -DKLOGG_USE_SENTRY)
    target_sources(klogg_crash_handler PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/crashhandler.cpp)
    target_link_libraries(klogg_crash_handler PUBLIC sentry crashpad_client)
endif()

if (KLOGG_USE_LTO)
set_property(TARGET klogg_crash_handler PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()