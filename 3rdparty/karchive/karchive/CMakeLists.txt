set(HAVE_BZIP2_SUPPORT 1)
set(HAVE_XZ_SUPPORT 1)

#configure_file(src/config-compression.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-compression.h)

set(karchive_SRCS
    src/loggingcategory.cpp
    src/karchive.cpp
    src/kar.cpp
    src/kcompressiondevice.cpp
    src/kfilterbase.cpp
    src/kfilterdev.cpp
    src/kgzipfilter.cpp
    src/klimitediodevice.cpp
    src/knonefilter.cpp
    src/ktar.cpp
    src/kzip.cpp
    src/krcc.cpp
    src/k7zip.cpp
    src/kbzip2filter.cpp
    src/kxzfilter.cpp
)

add_library(klogg_karchive STATIC ${karchive_SRCS} ${karchive_OPTIONAL_SRCS})

set_target_properties(klogg_karchive PROPERTIES AUTOMOC ON)

target_include_directories(klogg_karchive PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(klogg_karchive PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(klogg_karchive Qt${QT_VERSION_MAJOR}::Core cmzlib cmbzip2 cmliblzma)

target_compile_definitions(klogg_karchive PRIVATE -DQT_NO_CAST_FROM_ASCII)
target_compile_definitions(klogg_karchive PUBLIC -DLZMA_API_STATIC)
target_compile_definitions(klogg_karchive PUBLIC -DKLOGG_KARCHIVE)

if (MSVC)
	target_compile_definitions(klogg_karchive PRIVATE -D_SCL_SECURE_NO_WARNINGS)
else ()
        target_compile_options(klogg_karchive PRIVATE
            -Wall
            -Wno-long-long -Wno-variadic-macros
            -Wno-pedantic -Wno-unused-parameter -Wno-shadow
            -Wno-implicit-fallthrough)
endif()


