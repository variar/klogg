add_library(logdata STATIC
         ${CMAKE_CURRENT_SOURCE_DIR}/include/abstractlogdata.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/blockpool.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/compressedlinestorage.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/encodingdetector.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/linepositionarray.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/loadingstatus.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/logdata.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/logdataworkerthread.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/logfiltereddata.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/logfiltereddataworkerthread.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/marks.h
         ${CMAKE_CURRENT_SOURCE_DIR}/include/linetypes.h

        ${CMAKE_CURRENT_SOURCE_DIR}/src/abstractlogdata.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/blockpool.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/compressedlinestorage.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/encodingdetector.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/logdata.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/logdataworkerthread.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/logfiltereddata.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/logfiltereddataworkerthread.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/marks.cpp

)

target_include_directories(logdata PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(logdata PUBLIC Qt5::Core Qt5::Concurrent tbb_static moodycamel named_type absl::optional utils filewatch uchardet)
set_property(TARGET logdata PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
