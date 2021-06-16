cmake_minimum_required(VERSION 3.12)

project(klogg
        VERSION 21.06.0
        DESCRIPTION "klogg log viewer"
        LANGUAGES C CXX ASM)

set(PROJECT_HOMEPAGE_URL "https://github.com/variar/klogg")

string(TIMESTAMP PRODUCT_CURRENT_YEAR "%Y")
set(COMPANY "Anton Filimonov")
set(COPYRIGHT "Copyright (c) ${PRODUCT_CURRENT_YEAR} Nicolas Bonnefon, Anton Filimonov and other contributors.")
set(IDENTIFIER "com.github.variar.klogg")
set(PROJECT_DESCRIPTION "${PROJECT_NAME} log viewer")

option(KLOGG_BUILD_TESTS "Build tests" ON)
option(KLOGG_USE_LTO "Use link time optimization" ON)
option(KLOGG_USE_SENTRY "Use Sentry" OFF)
option(KLOGG_GENERIC_CPU "Build for generic CPU" OFF)

if(APPLE)
option(KLOGG_OVERRIDE_MALLOC "Use malloc global override" OFF)
else()
option(KLOGG_OVERRIDE_MALLOC "Use malloc global override" ON)
endif()

option(KLOGG_USE_MIMALLOC "Use mimalloc for malloc override" ON)

if (CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(KLOGG_ARCH 32)
else()
    set(KLOGG_ARCH 64)
endif()

if(WIN32)
    if(KLOGG_ARCH EQUAL 64)
        option(KLOGG_USE_HYPERSCAN "Use Hyperscan" ON)
    else()
        option(KLOGG_USE_HYPERSCAN "Use Hyperscan" OFF)
    endif()
else()
    option(KLOGG_USE_HYPERSCAN "Use Hyperscan" ON)
endif()

set(BUILD_VERSION $ENV{KLOGG_VERSION} CACHE STRING "build version")
set(BUILD_NUMBER "" CACHE STRING "build number")

set(NOTICE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/NOTICE)
set(COPYING_FILE ${CMAKE_CURRENT_SOURCE_DIR}/COPYING)

set(README_FILE ${CMAKE_CURRENT_SOURCE_DIR}/README.md)

set(DOCUMENTATION_FILE ${CMAKE_CURRENT_SOURCE_DIR}/DOCUMENTATION.md)
set(DOCUMENTATION_HTML ${CMAKE_BINARY_DIR}/generated/documentation.html)
set(DOCUMENTATION_RESOURCE ${CMAKE_BINARY_DIR}/generated/documentation.qrc)

if (WIN32)
    set(ICON_NAME klogg.ico)
elseif(APPLE)
    set(ICON_NAME klogg.icns)
endif()

if (NOT ${ICON_NAME} STREQUAL "")
    set(ICON_FILE ${CMAKE_CURRENT_SOURCE_DIR}/Resources/${ICON_NAME})
endif()

if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET 10.14)
endif()

if (MSVC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.14)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /experimental:external /external:W0 ")
    set(CMAKE_INCLUDE_SYSTEM_FLAG_CXX "/external:I ")
endif()

set(out_dir "${CMAKE_BINARY_DIR}/output")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${out_dir}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${out_dir}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${out_dir}")

set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)	
set(CMAKE_CXX_STANDARD_REQUIRED ON)	
#set(CMAKE_CXX_EXTENSIONS OFF)

include(StandardProjectSettings)
include(ucm)
include(prepare_version)
include(FindPkgConfig)
include(GNUInstallDirs)

if(MSVC)
    IF (CMAKE_CXX_FLAGS MATCHES "/W[0-4]")	
        STRING(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")	
    ELSE (CMAKE_CXX_FLAGS MATCHES "/W[0-4]")	
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")	
    ENDIF (CMAKE_CXX_FLAGS MATCHES "/W[0-4]")	

    IF (CMAKE_C_FLAGS MATCHES "/W[0-4]")	
        STRING(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")	
    ELSE (CMAKE_C_FLAGS MATCHES "/W[0-4]")	
        SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")	
    ENDIF (CMAKE_C_FLAGS MATCHES "/W[0-4]")
else()
    ucm_add_flags(CXX "-fno-sized-deallocation")
    ucm_add_flags(C CXX "-mmmx -msse -msse2 -msse3 -mssse3 -mpopcnt")
    if (KLOGG_GENERIC_CPU)
        ucm_add_flags(C CXX "-march=x86-64 -mtune=generic")
    else()
        ucm_add_flags(C CXX "-march=native -mtune=generic")
    endif()
endif(MSVC)


if (WIN32)
    set(SYMS_TOOLS ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/sentry/dump_syms/win)
elseif(APPLE)
    set(SYMS_TOOLS ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/sentry/dump_syms/mac)
else()
    set(SYMS_TOOLS ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/sentry/dump_syms/linux)
endif()

# standard compiler warnings
# Link this 'library' to use the warnings specified in CompilerWarnings.cmake
include(CompilerWarnings)
add_library(project_warnings INTERFACE)
set_project_warnings(project_warnings)

# Link this 'library' to set the c++ standard / compile-time options requested
include(CompilerFlags)
add_library(project_options INTERFACE)
set_project_compile_flags(project_options)

# sanitizer options if supported by compiler
include(Sanitizers)
enable_sanitizers(project_options)

# allow for static analysis options
include(StaticAnalyzers)

ucm_print_flags()

find_package(Qt5 COMPONENTS Core Widgets Concurrent Network Xml REQUIRED)

message("Qt version ${Qt5Core_VERSION}")

set(CMAKE_AUTOMOC OFF)
set(CMAKE_INCLUDE_CURRENT_DIR OFF)
set(BUILD_SHARED_LIBS OFF)

add_custom_target(ci_build)

add_subdirectory(3rdparty)
add_subdirectory(src)
add_subdirectory(website)


enable_testing()
add_test(
    NAME klogg_smoke
    COMMAND klogg -platform offscreen -v
)

if (${KLOGG_BUILD_TESTS})
    add_subdirectory(tests)
endif()

# Installation and packaging
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY true)
set(CPACK_SOURCE_IGNORE_FILES "build_root;.git")

if(UNIX)
    include(InstallRequiredSystemLibraries)

    SET(CPACK_OUTPUT_FILE_PREFIX "packages")
    SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
    SET(CPACK_PACKAGE_VENDOR ${COMPANY})
    SET(CPACK_PACKAGE_CONTACT "anton.filimonov@gmail.com")
    SET(CPACK_PACKAGE_DESCRIPTION_FILE "${README_FILE}")
    SET(CPACK_RESOURCE_FILE_LICENSE "${COPYING_FILE}")
    SET(CPACK_PACKAGE_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_TWEAK}")
    SET(CPACK_STRIP_FILES "bin/klogg")

    if (APPLE)
        set(CPACK_GENERATOR "DragNDrop")
        set(CPACK_DMG_FORMAT "UDBZ")
        set(CPACK_DMG_VOLUME_NAME "${PROJECT_NAME}")
        set(CPACK_SYSTEM_NAME "OSX")
        set(CPACK_PACKAGE_ICON "${ICON_FILE}")
        set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_CURRENT_SOURCE_DIR}/packaging/osx/dmg_background.tif")
        set(CPACK_DMG_DS_STORE_SETUP_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/packaging/osx/dmg_setup.scpt")

    else()
        install(FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/src/app/images/hicolor/16x16/klogg.png
            DESTINATION share/icons/hicolor/16x16/apps
        )

        install(FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/src/app/images/hicolor/32x32/klogg.png
            DESTINATION share/icons/hicolor/32x32/apps
        )

        install(FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/src/app/images/hicolor/scalable/klogg.svg
            DESTINATION share/icons/hicolor/scalable/apps
        )

        install(FILES
            ${COPYING_FILE} 
            ${NOTICE_FILE}
            ${README_FILE}
            ${DOCUMENTATION_HTML}
            ${DOCUMENTATION_FILE}
            DESTINATION ${CMAKE_INSTALL_DOCDIR})

        install(FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/klogg.desktop
            DESTINATION share/applications
        )

        if(KLOGG_OVERRIDE_MALLOC AND NOT KLOGG_USE_MIMALLOC)
            install(TARGETS klogg_tbbmalloc
                    LIBRARY
                    DESTINATION ${CMAKE_INSTALL_LIBDIR})
            install(TARGETS klogg_tbbmalloc_proxy
                    LIBRARY
                    DESTINATION ${CMAKE_INSTALL_LIBDIR})
        endif()

        set(CPACK_GENERATOR DEB)

        #deb
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
        set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
        set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "http://github.com/variar/klogg")
        set(CPACK_DEBIAN_PACKAGE_DEPENDS ${CPACK_DEBIAN_PACKAGE_DEPENDS}
                                            "libqt5widgets5 (>=5.9.5), libqt5concurrent5 (>=5.9.5), libqt5xml5 (>=5.9.5)")


        #RPM
        set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
        set(CPACK_RPM_PACKAGE_LICENSE "GPLv3")
        set(CPACK_RPM_PACKAGE_REQUIRES "qt5-qtbase >= 5.9.5")
        set(CPACK_RPM_PACKAGE_URL "http://github.com/variar/klogg")
        set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION 
            /usr/share/applications
            /usr/share/icons
            /usr/share/icons/hicolor
            /usr/share/icons/hicolor/16x16
            /usr/share/icons/hicolor/16x16/apps
            /usr/share/icons/hicolor/32x32
            /usr/share/icons/hicolor/32x32/apps
            /usr/share/icons/hicolor/scalable
            /usr/share/icons/hicolor/scalable/apps
        )

        if(KLOGG_OVERRIDE_MALLOC AND NOT KLOGG_USE_MIMALLOC)
            set(CPACK_RPM_PACKAGE_PROVIDES "libklogg_tbbmalloc.so()(64bit),libklogg_tbbmalloc_proxy.so()(64bit)")
        endif()

    endif()
    INCLUDE(CPack)
endif()

# for IDE grouping
add_custom_target(installation SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/packaging/windows/klogg.nsi
    ${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/klogg.desktop
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/MacOSXBundleInfo.plist.in
)

add_custom_target(docs SOURCES
    ${README_FILE} ${COPYING_FILE} ${NOTICE_FILE} ${DOCUMENTATION_FILE}
)

add_custom_target(stuff SOURCES
   .clang-format
   .clang-tidy
)


