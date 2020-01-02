add_library(utils STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/persistentinfo.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/configuration.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/crash_tracer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/downloader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/decompressor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/atomicflag.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/configuration.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/log.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/uuid.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/perfcounter.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/persistable.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/persistentinfo.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/openfilehelper.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/crash_tracer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/downloader.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/decompressor.h)

set_target_properties(utils PROPERTIES AUTOMOC ON)

target_include_directories(utils PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(utils PUBLIC
        Qt5::Gui
        Qt5::Network
        Qt5::Concurrent
        absl::optional
        absl::symbolize
        absl::failure_signal_handler
        klogg_karchive
        plog)
