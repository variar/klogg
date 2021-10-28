add_library(kdtoolbox STATIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/KDSignalThrottler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/KDSignalThrottler.h)

set_target_properties(kdtoolbox PROPERTIES AUTOMOC ON)

target_include_directories(kdtoolbox SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(kdtoolbox Qt${QT_VERSION_MAJOR}::Core)