set(QAPPLICATION_CLASS QApplication)

include(CPM)

set(_TMP_CPM_USE_LOCAL_PACKAGES ${CPM_USE_LOCAL_PACKAGES})

cpmaddpackage("gh:simdutf/simdutf@9.3.3")
cpmaddpackage("gh:foonathan/type_safe@0.2.4")
cpmaddpackage("gh:RoaringBitmap/CRoaring@5.5.5")
cpmaddpackage("gh:lemire/streamvbyte@3.0.2")

if(APPLE)
  cpmaddpackage(
    NAME
    macdeployqtfix
    GITHUB_REPOSITORY
    arl/macdeployqtfix
    GIT_TAG
    631361abd994640023c83b539735d8373744f9dd
    DOWNLOAD_ONLY
    YES
  )
  if(macdeployqtfix_ADDED)
    configure_file(${macdeployqtfix_SOURCE_DIR}/macdeployqtfix.py ${CMAKE_BINARY_DIR}/macdeployqtfix.py COPYONLY)
  endif()
endif()

cpmaddpackage(
  NAME
  maddy
  GITHUB_REPOSITORY
  variar/maddy
  GIT_TAG
  602e26613e624535e2de883b3f2c98a16729d1d4
  EXCLUDE_FROM_ALL
  YES
)

if(KLOGG_USE_HYPERSCAN)
  cpmaddpackage(
    NAME
    hyperscan
    GITHUB_REPOSITORY
    variar/hyperscan
    GIT_TAG
    0931a40e0cf1d7f92189bc546c3491ed5c113f8b
    EXCLUDE_FROM_ALL
    YES
  )
  message("Adding alias for hyperscan")
  add_library(klogg_hyperscan INTERFACE)
  target_link_libraries(klogg_hyperscan INTERFACE hs)
  target_include_directories(klogg_hyperscan INTERFACE ${hyperscan_SOURCE_DIR}/src)
elseif(KLOGG_USE_VECTORSCAN)
  cpmaddpackage(
    NAME
    vectorscan
    GITHUB_REPOSITORY
    VectorCamp/vectorscan
    GIT_TAG
    vectorscan/5.4.14
    EXCLUDE_FROM_ALL
    YES
    OPTIONS
    "BUILD_STATIC_LIBS ON"
    "BUILD_UNIT OFF"
    "BUILD_TOOLS OFF"
    "BUILD_EXAMPLES OFF"
    "BUILD_BENCHMARKS OFF"
    "BUILD_DOC OFF"
    "BUILD_CHIMERA OFF"
    "BUIlD_AVX2 OFF"
    "BUIlD_AVX512 OFF"
    "BUIlD_AVX512VBMI OFF"
    "FAT_RUNTIME OFF"
  )
 
  message("Adding alias for vectorscan")
  add_library(klogg_vectorscan INTERFACE)
  target_link_libraries(klogg_vectorscan INTERFACE hs)
  target_include_directories(klogg_vectorscan INTERFACE 
    ${vectorscan_SOURCE_DIR}/src
    ${vectorscan_BINARY_DIR}
  )
endif()

cpmaddpackage(
  NAME
  Uchardet
  GIT_REPOSITORY
  https://gitlab.freedesktop.org/uchardet/uchardet
  VERSION
  0.0.8
  EXCLUDE_FROM_ALL
  YES
  OPTIONS
  "BUILD_BINARY OFF"
)
if(Uchardet_ADDED)
  message("Adding alias for uchardet")
  add_library(klogg_uchardet_wrapper INTERFACE)
  target_link_libraries(klogg_uchardet_wrapper INTERFACE libuchardet)
  target_include_directories(klogg_uchardet_wrapper INTERFACE ${Uchardet_SOURCE_DIR}/src)
else()
  add_library(klogg_uchardet_wrapper INTERFACE)
  target_link_libraries(klogg_uchardet_wrapper INTERFACE ${UCHARDET_LIBRARY})
  target_include_directories(klogg_uchardet_wrapper INTERFACE ${UCHARDET_INCLUDE_DIR})
endif()

if(${QT_VERSION_MAJOR} EQUAL 6) 
set(CPM_USE_LOCAL_PACKAGES OFF)
endif()

# Qt >= 6.9 no longer accepts QFlags in QString::arg(), patch karchive sources
set(_klogg_karchive_patches)
if(KLOGG_QT_VERSION VERSION_GREATER_EQUAL 6.9)
  set(_klogg_karchive_patches PATCHES "${CMAKE_CURRENT_LIST_DIR}/../cmake/patches/karchive-qt6.9-qstring-arg.patch")
endif()

cpmaddpackage(
  NAME
  KF5Archive
  GITHUB_REPOSITORY
  variar/klogg_karchive
  GIT_TAG
  f546bf6ae66a8d34b43da5a41afcfbf4e1a47906
  EXCLUDE_FROM_ALL
  YES
  ${_klogg_karchive_patches}
)
if(NOT KF5Archive_ADDED)
  find_package(KF5Archive)
  add_library(klogg_karchive INTERFACE)
  target_link_libraries(klogg_karchive INTERFACE KF5::Archive)
endif()

if(${QT_VERSION_MAJOR} EQUAL 6)
message("Resetting CPM_USE_LOCAL_PACKAGES to ${_TMP_CPM_USE_LOCAL_PACKAGES}")
set(CPM_USE_LOCAL_PACKAGES ${_TMP_CPM_USE_LOCAL_PACKAGES})
endif()

# robin_hood.h relies on <cstdint> being included transitively, which is no longer
# the case with GCC 15 (Ubuntu 26.04). MSVC is not affected and avoids CRLF patch issues.
set(_klogg_robin_hood_patches)
if(NOT MSVC)
  set(_klogg_robin_hood_patches PATCHES "${CMAKE_CURRENT_LIST_DIR}/../cmake/patches/robin_hood-gcc15-cstdint.patch")
endif()

cpmaddpackage(
  NAME
  robin_hood
  GITHUB_REPOSITORY
  martinus/robin-hood-hashing
  GIT_TAG
  3.11.5
  EXCLUDE_FROM_ALL
  YES
  ${_klogg_robin_hood_patches}
)
if(NOT TARGET robin_hood)
  message("Adding imported target for robin_hood")
  add_library(robin_hood INTERFACE)
  target_include_directories(robin_hood INTERFACE ${ROBIN_HOOD_INCLUDE_DIRS})
endif(NOT TARGET robin_hood)

if(KLOGG_BUILD_TESTS)
  cpmaddpackage("gh:bombela/backward-cpp@1.6")
  cpmaddpackage(
    NAME
    Catch2
    GITHUB_REPOSITORY
    catchorg/Catch2
    VERSION
    3.16.1
    EXCLUDE_FROM_ALL
    YES
  )
  if(NOT TARGET Catch2)
    message("Adding imported target for catch2")
    add_library(Catch2 INTERFACE)
    target_include_directories(Catch2 INTERFACE ${CATCH2_INCLUDE_DIRS})
  endif(NOT TARGET Catch2)
endif()

cpmaddpackage(
  NAME
  KDSingleApplication
  GITHUB_REPOSITORY
  variar/KDSingleApplication
  GIT_TAG
  5b30db30266f92bc01f1439777803ce8dbf16c79
  DOWNLOAD_ONLY
  YES
)
if(KDSingleApplication_ADDED)
  add_library(
    kdsingleapp STATIC ${KDSingleApplication_SOURCE_DIR}/src/kdsingleapplication.cpp
                     ${KDSingleApplication_SOURCE_DIR}/src/kdsingleapplication_localsocket.cpp
  )

  target_include_directories(kdsingleapp PUBLIC ${KDSingleApplication_SOURCE_DIR}/src)
  target_link_libraries(kdsingleapp Qt${QT_VERSION_MAJOR}::Core)
  target_compile_definitions(kdsingleapp PUBLIC KDSINGLEAPPLICATION_STATIC_BUILD)

  set_target_properties(kdsingleapp PROPERTIES AUTOMOC ON)
endif()


cpmaddpackage(
  NAME
  xxHash
  GITHUB_REPOSITORY
  Cyan4973/xxHash
  VERSION
  0.8.4
  DOWNLOAD_ONLY
  YES
)
if(xxHash_ADDED)
  set(XXHASH_BUILD_XXHSUM OFF)
  add_subdirectory(${xxHash_SOURCE_DIR}/build/cmake ${CMAKE_BINARY_DIR}/xxhash EXCLUDE_FROM_ALL)
endif()

cpmaddpackage(
  NAME
  whereami
  GITHUB_REPOSITORY
  gpakosz/whereami
  GIT_TAG
  dcb52a058dc14530ba9ae05e4339bd3ddfae0e0e
  DOWNLOAD_ONLY
  YES
)
if(whereami_ADDED)
  add_library(whereami STATIC ${whereami_SOURCE_DIR}/src/whereami.h ${whereami_SOURCE_DIR}/src/whereami.c)
  target_include_directories(whereami PUBLIC ${whereami_SOURCE_DIR}/src)
endif()

cpmaddpackage(
  NAME
  exprtk
  GITHUB_REPOSITORY
  variar/klogg_exprtk
  GIT_TAG
  1f9f4cd7d2620b7b24232de9ea22908d63913459
  DOWNLOAD_ONLY
  YES
)
if(exprtk_ADDED)
  add_library(exprtk INTERFACE)
  target_link_libraries(exprtk INTERFACE robin_hood)
  target_include_directories(exprtk SYSTEM INTERFACE ${exprtk_SOURCE_DIR})
  target_compile_definitions(
    exprtk
    INTERFACE -Dexprtk_disable_caseinsensitivity
              -Dexprtk_disable_comments
              -Dexprtk_disable_break_continue
              -Dexprtk_disable_return_statement
              -Dexprtk_disable_superscalar_unroll
              -Dexprtk_disable_rtl_io_file
              -Dexprtk_disable_rtl_vecops
              -Dexprtk_disable_string_capabilities
  )
endif()

cpmaddpackage(
  NAME
  KDToolBox
  GITHUB_REPOSITORY
  KDAB/KDToolBox
  GIT_TAG
  8e28921e436fd805904118d04f1f9a8428ca1408
  DOWNLOAD_ONLY
  YES
)
if(KDToolBox_ADDED)
  add_library(
    kdtoolbox STATIC ${KDToolBox_SOURCE_DIR}/qt/KDSignalThrottler/src/KDSignalThrottler.cpp
                     ${KDToolBox_SOURCE_DIR}/qt/KDSignalThrottler/src/KDSignalThrottler.h
  )

  target_include_directories(kdtoolbox PUBLIC ${KDToolBox_SOURCE_DIR}/qt/KDSignalThrottler/src)
  target_link_libraries(kdtoolbox Qt${QT_VERSION_MAJOR}::Core)
  set_target_properties(kdtoolbox PROPERTIES AUTOMOC ON)
endif()

cpmaddpackage(
  NAME
  efsw
  GITHUB_REPOSITORY
  SpartanJ/efsw
  GIT_TAG
  1.7.3
  EXCLUDE_FROM_ALL
  YES
  OPTIONS
  "BUILD_TEST_APP OFF"
)
if(efsw_ADDED)
  target_compile_definitions(efsw PRIVATE EFSW_FSEVENTS_NOT_SUPPORTED)
endif()

if(WIN32)
  set(BUILD_SHARED_LIBS ON)
endif()
cpmaddpackage(
  NAME
  tbb
  GITHUB_REPOSITORY
  variar/oneTBB
  GIT_TAG
  d1d43adf6af117d3ab4ee3a14a5fa129e0690290
  EXCLUDE_FROM_ALL
  YES
  OPTIONS
  "TBB_TEST OFF"
  "TBB_EXAMPLES OFF"
  "TBB_STRICT OFF"
)

if(WIN32)
  set(BUILD_SHARED_LIBS OFF)
endif()

set(CPM_USE_LOCAL_PACKAGES OFF)
cpmaddpackage(
  NAME
  mimalloc
  GITHUB_REPOSITORY
  microsoft/mimalloc
  VERSION
  3.6.1
  EXCLUDE_FROM_ALL
  YES
  OPTIONS
  "MI_BUILD_TESTS OFF"
  "MI_SECURE OFF"
  "MI_BUILD_SHARED OFF"
  "MI_BUILD_STATIC ON"
  "MI_BUILD_OBJECT OFF"
  "MI_OVERRIDE OFF"
)
if(mimalloc_ADDED)
  message("Adding alias for mimalloc")
  add_library(klogg_mimalloc_wrapper INTERFACE)
  target_link_libraries(klogg_mimalloc_wrapper INTERFACE mimalloc-static)
  target_include_directories(klogg_mimalloc_wrapper INTERFACE ${mimalloc_SOURCE_DIR}/include)
else()
  add_library(klogg_mimalloc_wrapper INTERFACE)
  target_link_libraries(klogg_mimalloc_wrapper INTERFACE ${MIMALLOC_LIBRARY})
  target_include_directories(klogg_mimalloc_wrapper INTERFACE ${MIMALLOC_INCLUDE_DIR})
endif()

set(CPM_USE_LOCAL_PACKAGES ${_TMP_CPM_USE_LOCAL_PACKAGES})

set(klogg_cpm_targets
    xxhash
    Catch2
    roaring
    roaring-headers-cpp
    libuchardet
    robin_hood
    whereami
    simdutf
    efsw
    SingleApplication
    hs
    tbb
    exprtk
    type_safe
    mimalloc-static
    streamvbyte
    FastPFor
)
foreach(target ${klogg_cpm_targets})
  if(TARGET ${target})
    get_property(
      ${target}_include_dirs
      TARGET ${target}
      PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    )
    #message("Marking ${${target}_include_dirs} as system incldue")
    set_property(TARGET ${target} PROPERTY INTERFACE_SYSTEM_INCLUDE_DIRECTORIES ${${target}_include_dirs})
  endif()
endforeach()
