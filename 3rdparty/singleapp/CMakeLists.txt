find_package(Qt5Network REQUIRED)

add_library(singleapp STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/singleapp/singleapplication.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/singleapplication.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/singleapplication_p.cpp)

set_property(TARGET singleapp PROPERTY AUTOMOC ON)

target_include_directories(singleapp SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(singleapp SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/singleapp)

target_link_libraries(singleapp PUBLIC Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network)
target_compile_definitions(singleapp PUBLIC QAPPLICATION_CLASS=QApplication)
