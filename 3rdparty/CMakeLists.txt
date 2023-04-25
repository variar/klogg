set(QAPPLICATION_CLASS QApplication)

include(CPM)

cpmaddpackage("gh:simdutf/simdutf@3.2.8")

if(APPLE)
  cpmaddpackage(
    NAME
    macdeployqtfix
    GITHUB_REPOSITORY
    arl/macdeployqtfix
    GIT_TAG
    ffe980011dd7a08ac2bc79dbd5ac86a62b1c1f05
    DOWNLOAD_ONLY
    YES
  )
  if(macdeployqtfix_ADDED)
    configure_file(${macdeployqtfix_SOURCE_DIR}/macdeployqtfix.py ${CMAKE_BINARY_DIR}/macdeployqtfix.py COPYONLY)
  endif()
endif()

cpmaddpackage(
  NAME
  CRoaring
  GITHUB_REPOSITORY
  variar/CRoaring
  GIT_TAG
  d582cd224ebc2f5a1585035a8f5ce306052e8ad1
  EXCLUDE_FROM_ALL
  YES
  OPTIONS
  "ENABLE_ROARING_TESTS OFF"
)

cpmaddpackage(
  NAME
  maddy
  GITHUB_REPOSITORY
  variar/maddy
  GIT_TAG
  67d331c59d3abc4e92eecc5874836a600dbfaf58
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
  if(hyperscan_ADDED)
    message("Adding alias for hyperscan")
    add_library(hyperscan_wrapper INTERFACE)
    target_link_libraries(hyperscan_wrapper INTERFACE hs)
    target_include_directories(hyperscan_wrapper INTERFACE ${hyperscan_SOURCE_DIR}/src)
  else()
    add_library(hyperscan_wrapper INTERFACE)
    target_link_libraries(hyperscan_wrapper INTERFACE ${HYPERSCAN_LIBRARY})
    target_include_directories(hyperscan_wrapper INTERFACE ${HYPERSCAN_INCLUDE_DIR})
  endif()
endif()

cpmaddpackage(
  NAME
  NamedType
  GITHUB_REPOSITORY
  joboccara/NamedType
  GIT_TAG
  215f039574a7d5111f1a131def337f142029f896
  DOWNLOAD_ONLY
  YES
)
if(NOT TARGET NamedType)
  message("Adding imported target for NamedType")
  add_library(NamedType INTERFACE)
  target_include_directories(NamedType SYSTEM INTERFACE ${NamedType_SOURCE_DIR})
endif()

cpmaddpackage(
  NAME
  Uchardet
  GIT_REPOSITORY
  https://gitlab.freedesktop.org/uchardet/uchardet
  VERSION
  0.0.7
  EXCLUDE_FROM_ALL
  YES
  OPTIONS
  "BUILD_BINARY OFF"
)
if(Uchardet_ADDED)
  message("Adding alias for uchardet")
  add_library(uchardet_wrapper INTERFACE)
  target_link_libraries(uchardet_wrapper INTERFACE libuchardet)
  target_include_directories(uchardet_wrapper INTERFACE ${Uchardet_SOURCE_DIR}/src)
else()
  add_library(uchardet_wrapper INTERFACE)
  target_link_libraries(uchardet_wrapper INTERFACE ${UCHARDET_LIBRARY})
  target_include_directories(uchardet_wrapper INTERFACE ${UCHARDET_INCLUDE_DIR})
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
)
if(NOT KF5Archive_ADDED)
  find_package(KF5Archive)
  add_library(klogg_karchive INTERFACE)
  target_link_libraries(klogg_karchive INTERFACE KF5::Archive)
endif()

cpmaddpackage(
  NAME
  robin_hood
  GITHUB_REPOSITORY
  martinus/robin-hood-hashing
  GIT_TAG
  3.11.2
  EXCLUDE_FROM_ALL
  YES
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
    2.13.8
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
  SingleApplication
  GITHUB_REPOSITORY
  itay-grudev/SingleApplication
  VERSION
  3.3.4
  DOWNLOAD_ONLY
  YES
)
if(SingleApplication_ADDED)
  add_subdirectory(${SingleApplication_SOURCE_DIR} ${CMAKE_BINARY_DIR}/SingleApplication EXCLUDE_FROM_ALL)
  set_property(TARGET SingleApplication PROPERTY AUTOMOC ON)
endif()

cpmaddpackage(
  NAME
  xxHash
  GITHUB_REPOSITORY
  Cyan4973/xxHash
  VERSION
  0.8.1
  DOWNLOAD_ONLY
  YES
)
if(xxHash_ADDED)
  set(XXHASH_BUILD_ENABLE_INLINE_API OFF)
  set(XXHASH_BUILD_XXHSUM OFF)
  add_subdirectory(${xxHash_SOURCE_DIR}/cmake_unofficial ${CMAKE_BINARY_DIR}/xxhash EXCLUDE_FROM_ALL)
endif()

cpmaddpackage(
  NAME
  whereami
  GITHUB_REPOSITORY
  gpakosz/whereami
  GIT_TAG
  ba364cd54fd431c76c045393b6522b4bff547f50
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
  253a6c087c626cd477e15c9dae4b0e3ec27afaee
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
  2e73f8e96779a2895c6242d66cadb3a15aa5e192
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
  281c5b4ae2f1a610a13ba9e812350bb01bb88365
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

if(TARGET tbbmalloc_proxy)
  add_library(tbbmalloc_proxy_wrapper ALIAS tbbmalloc_proxy)
else()
  add_library(tbbmalloc_proxy_wrapper INTERFACE)
  find_package(tbbmalloc_proxy)
  target_link_libraries(tbbmalloc_proxy_wrapper INTERFACE ${TBB_MALLOC_PROXY_LIBRARIES})
endif()

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
    NamedType
    hs
    tbb
    exprtk
)
foreach(target ${klogg_cpm_targets})
  if(TARGET ${target})
    get_property(
      ${target}_include_dirs
      TARGET ${target}
      PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    )
    set_property(TARGET ${target} PROPERTY INTERFACE_SYSTEM_INCLUDE_DIRECTORIES ${${target}_include_dirs})
  endif()
endforeach()

if(KLOGG_OVERRIDE_MALLOC)
  add_library(klogg_malloc_proxy INTERFACE)
  target_compile_definitions(klogg_malloc_proxy INTERFACE -DKLOGG_OVERRIDE_MALLOC)

  if(NOT WIN32 AND KLOGG_USE_MIMALLOC)
    target_compile_definitions(klogg_malloc_proxy INTERFACE -DKLOGG_USE_MIMALLOC)

    cpmaddpackage(
      NAME
      mimalloc
      GITHUB_REPOSITORY
      microsoft/mimalloc
      VERSION
      2.0.5
      EXCLUDE_FROM_ALL
      YES
      OPTIONS
      "MI_BUILD_TESTS OFF"
      "MI_SECURE ON"
      "MI_BUILD_SHARED OFF"
      "MI_BUILD_STATIC OFF"
      "MI_BUILD_OBJECT ON"
    )
    if(mimalloc_ADDED)
      target_link_libraries(klogg_malloc_proxy INTERFACE mimalloc-obj)
    endif()
  else()
    target_compile_definitions(klogg_malloc_proxy INTERFACE -DKLOGG_USE_TBBMALLOC)
    target_link_libraries(klogg_malloc_proxy INTERFACE tbbmalloc_proxy_wrapper)
  endif()
endif()

if(KLOGG_USE_SENTRY)
  set(SENTRY_BACKEND
      "crashpad"
      CACHE INTERNAL "" FORCE
  )
  set(SENTRY_TRANSPORT
      "none"
      CACHE INTERNAL "" FORCE
  )
  set(SENTRY_BUILD_EXAMPLES
      OFF
      CACHE INTERNAL "" FORCE
  )
  set(CRASHPAD_ENABLE_INSTALL
      OFF
      CACHE INTERNAL "" FORCE
  )
  set(CRASHPAD_ENABLE_INSTALL_DEV
      OFF
      CACHE INTERNAL "" FORCE
  )
  add_subdirectory(sentry)
endif(KLOGG_USE_SENTRY)
