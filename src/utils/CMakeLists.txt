add_library(klogg_utils INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/atomicflag.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/uuid.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/dispatch_to.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/perfcounter.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/openfilehelper.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/progress.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/synchronization.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/overload_visitor.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/resourcewrapper.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/crc32.h)

set_target_properties(klogg_utils PROPERTIES AUTOMOC ON)

target_include_directories(klogg_utils INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(klogg_utils INTERFACE
        project_options
        project_warnings

        klogg_version
        klogg_logging

        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Network
        Qt${QT_VERSION_MAJOR}::Concurrent

        whereami
)


