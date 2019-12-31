project(bzip2)
add_definitions(-D_FILE_OFFSET_BITS=64)
add_library(cmbzip2 STATIC
  blocksort.c huffman.c crctable.c randtable.c compress.c decompress.c bzlib.c)

if (MSVC)
	target_compile_definitions(cmbzip2 PRIVATE -D_SCL_SECURE_NO_WARNINGS)
else ()
        target_compile_options(cmbzip2 PRIVATE
            -Wall
            -Wno-long-long -Wno-variadic-macros
            -Wno-pedantic -Wno-unused-parameter -Wno-shadow
            -Wno-implicit-fallthrough)
endif()

target_include_directories(cmbzip2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})