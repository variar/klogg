add_library(utils STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/persistentinfo.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/configuration.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/downloader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/decompressor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/klogg_version.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/readablesize.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/memory_info.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/cpu_info.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/issuereporter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/atomicflag.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/configuration.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/log.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/uuid.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/dispatch_to.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/perfcounter.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/persistable.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/persistentinfo.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/openfilehelper.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/downloader.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/decompressor.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/klogg_version.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/progress.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/readablesize.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/synchronization.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/memory_info.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/cpu_info.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/overload_visitor.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/resourcewrapper.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/styles.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/issuereporter.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/shortcuts.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/crc32.h)

set_target_properties(utils PROPERTIES AUTOMOC ON)

target_include_directories(utils PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(utils PUBLIC
        project_options
        project_warnings
        Qt5::Gui
        Qt5::Widgets
        Qt5::Network
        Qt5::Concurrent
        klogg_karchive
        whereami
        plog)

add_dependencies(utils generate_version)

