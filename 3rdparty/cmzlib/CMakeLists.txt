PROJECT(CMZLIB)

# source files for zlib
SET(ZLIB_SRCS
  adler32.c  compress.c  crc32.c  deflate.c  gzio.c   inffast.c
  inflate.c  inftrees.c  trees.c  uncompr.c  zutil.c
  )

CONFIGURE_FILE(${CMZLIB_SOURCE_DIR}/.NoDartCoverage
  ${CMZLIB_BINARY_DIR}/.NoDartCoverage)
CONFIGURE_FILE(${CMZLIB_SOURCE_DIR}/zlibDllConfig.h.in
  ${CMZLIB_BINARY_DIR}/zlibDllConfig.h)

FOREACH(name zlib zconf cm_zlib_mangle )
  CONFIGURE_FILE(${CMZLIB_SOURCE_DIR}/${name}.h
    ${CMZLIB_BINARY_DIR}/${name}.h COPYONLY)
ENDFOREACH(name)


ADD_LIBRARY(cmzlib STATIC ${ZLIB_SRCS})

if (MSVC)
	target_compile_definitions(cmzlib PRIVATE -D_SCL_SECURE_NO_WARNINGS)
else ()
        target_compile_options(cmzlib PRIVATE
            -Wall
            -Wno-long-long -Wno-variadic-macros
            -Wno-pedantic -Wno-unused-parameter -Wno-shadow
            -Wno-implicit-fallthrough)
endif()

target_include_directories(cmzlib PUBLIC "${CMZLIB_SOURCE_DIR}" "${CMZLIB_SOURCE_DIR}/.." "${CMZLIB_BINARY_DIR}/..")