set(QAPPLICATION_CLASS QApplication)

include(CPM)

# Imported targets from find_package() are directory-scoped by default; the rest
# of the tree (src/, tests/) links them from sibling directories, so promote
# them to global scope.
set(CMAKE_FIND_PACKAGE_TARGETS_GLOBAL ON)

# ---------------------------------------------------------------------------
# Dependencies provided by vcpkg (see ../vcpkg.json)
#
# Each find_package() below resolves to a vcpkg-installed config package. Thin
# INTERFACE "wrapper" targets re-expose them under the exact names the rest of
# the source tree already links against, so no changes are needed in src/.
#
# A few dependencies have no usable vcpkg port and stay on CPM (see the bottom
# of this file): streamvbyte, kdtoolbox (KDSignalThrottler), maddy (used as a
# doc-generation executable), macdeployqtfix, and KArchive (vcpkg only ships a
# Qt5 build which conflicts with klogg's Qt6).
# ---------------------------------------------------------------------------

# Create an INTERFACE target named ${alias} forwarding to ${real}, unless a
# target named ${alias} already exists (e.g. the config package defined it).
macro(klogg_alias alias real)
  if(NOT TARGET ${alias})
    add_library(${alias} INTERFACE)
    target_link_libraries(${alias} INTERFACE ${real})
  endif()
endmacro()

# --- simdutf ---------------------------------------------------------------
find_package(simdutf CONFIG REQUIRED)
klogg_alias(simdutf simdutf::simdutf)

# --- type_safe (foonathan) -------------------------------------------------
find_package(type_safe CONFIG REQUIRED)
if(NOT TARGET type_safe AND TARGET foonathan::type_safe)
  klogg_alias(type_safe foonathan::type_safe)
endif()

# --- CRoaring --------------------------------------------------------------
# klogg includes the C++ headers without the roaring/ prefix (<roaring.hh>),
# so expose the roaring subdirectory on the include path.
find_package(roaring CONFIG REQUIRED)
find_path(KLOGG_ROARING_INCLUDE_DIR NAMES roaring.hh PATH_SUFFIXES roaring)
foreach(roaring_target roaring roaring-headers roaring-headers-cpp)
  if(NOT TARGET ${roaring_target})
    add_library(${roaring_target} INTERFACE)
    target_link_libraries(${roaring_target} INTERFACE roaring::roaring)
    if(KLOGG_ROARING_INCLUDE_DIR)
      target_include_directories(${roaring_target} SYSTEM INTERFACE ${KLOGG_ROARING_INCLUDE_DIR})
    endif()
  endif()
endforeach()

# --- uchardet --------------------------------------------------------------
find_package(uchardet CONFIG REQUIRED)
add_library(klogg_uchardet_wrapper INTERFACE)
if(TARGET uchardet::libuchardet)
  target_link_libraries(klogg_uchardet_wrapper INTERFACE uchardet::libuchardet)
elseif(TARGET uchardet::uchardet)
  target_link_libraries(klogg_uchardet_wrapper INTERFACE uchardet::uchardet)
elseif(TARGET uchardet)
  target_link_libraries(klogg_uchardet_wrapper INTERFACE uchardet)
endif()
find_path(KLOGG_UCHARDET_INCLUDE_DIR NAMES uchardet.h PATH_SUFFIXES uchardet)
if(KLOGG_UCHARDET_INCLUDE_DIR)
  target_include_directories(klogg_uchardet_wrapper SYSTEM INTERFACE ${KLOGG_UCHARDET_INCLUDE_DIR})
endif()

# --- robin_hood ------------------------------------------------------------
find_package(robin_hood CONFIG REQUIRED)
klogg_alias(robin_hood robin_hood::robin_hood)

# --- xxHash ----------------------------------------------------------------
find_package(xxHash CONFIG REQUIRED)
klogg_alias(xxhash xxHash::xxhash)

# --- whereami --------------------------------------------------------------
find_package(unofficial-whereami CONFIG REQUIRED)
klogg_alias(whereami unofficial::whereami::whereami)

# --- efsw ------------------------------------------------------------------
find_package(efsw CONFIG REQUIRED)
klogg_alias(efsw efsw::efsw)

# --- oneTBB ----------------------------------------------------------------
find_package(TBB CONFIG REQUIRED)
klogg_alias(tbb TBB::tbb)

# --- mimalloc --------------------------------------------------------------
find_package(mimalloc CONFIG REQUIRED)
add_library(klogg_mimalloc_wrapper INTERFACE)
if(TARGET mimalloc-static)
  target_link_libraries(klogg_mimalloc_wrapper INTERFACE mimalloc-static)
elseif(TARGET mimalloc)
  target_link_libraries(klogg_mimalloc_wrapper INTERFACE mimalloc)
endif()

# --- exprtk (header only, no CMake config) ---------------------------------
add_library(exprtk INTERFACE)
target_link_libraries(exprtk INTERFACE robin_hood)
find_path(KLOGG_EXPRTK_INCLUDE_DIR NAMES exprtk.hpp)
if(KLOGG_EXPRTK_INCLUDE_DIR)
  target_include_directories(exprtk SYSTEM INTERFACE ${KLOGG_EXPRTK_INCLUDE_DIR})
endif()
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

# --- hyperscan / vectorscan (no CMake config, pkg-config/lib only) ---------
if(KLOGG_USE_HYPERSCAN)
  message("Adding alias for hyperscan (vcpkg)")
  find_library(KLOGG_HS_LIBRARY NAMES hs libhs REQUIRED)
  find_path(KLOGG_HS_INCLUDE_DIR NAMES hs.h PATH_SUFFIXES hs REQUIRED)
  add_library(klogg_hyperscan INTERFACE)
  target_link_libraries(klogg_hyperscan INTERFACE ${KLOGG_HS_LIBRARY})
  target_include_directories(klogg_hyperscan SYSTEM INTERFACE ${KLOGG_HS_INCLUDE_DIR})
elseif(KLOGG_USE_VECTORSCAN)
  message("Adding alias for vectorscan (vcpkg)")
  find_library(KLOGG_HS_LIBRARY NAMES hs libhs REQUIRED)
  find_path(KLOGG_HS_INCLUDE_DIR NAMES hs.h PATH_SUFFIXES hs REQUIRED)
  add_library(klogg_vectorscan INTERFACE)
  target_link_libraries(klogg_vectorscan INTERFACE ${KLOGG_HS_LIBRARY})
  target_include_directories(klogg_vectorscan SYSTEM INTERFACE ${KLOGG_HS_INCLUDE_DIR})
endif()

# --- tests: Catch2 (pinned to 2.x) + backward-cpp --------------------------
if(KLOGG_BUILD_TESTS)
  find_package(Catch2 CONFIG REQUIRED)
  klogg_alias(Catch2 Catch2::Catch2)
  # Provides the add_backward() helper used by tests/ui.
  find_package(Backward CONFIG REQUIRED)
endif()

# --- sentry (optional, off by default) -------------------------------------
if(KLOGG_USE_SENTRY)
  find_package(sentry CONFIG REQUIRED)
endif()

# ---------------------------------------------------------------------------
# Dependencies that stay on CPM (no usable vcpkg port)
# ---------------------------------------------------------------------------

if(APPLE)
  cpmaddpackage(
    NAME
    macdeployqtfix
    GITHUB_REPOSITORY
    arl/macdeployqtfix
    GIT_TAG
    df888505849d3c06d20a4338af276dfa7d11826a
    DOWNLOAD_ONLY
    YES
  )
  if(macdeployqtfix_ADDED)
    configure_file(${macdeployqtfix_SOURCE_DIR}/macdeployqtfix.py ${CMAKE_BINARY_DIR}/macdeployqtfix.py COPYONLY)
  endif()
endif()

# maddy is consumed as a documentation-conversion executable ($<TARGET_FILE:maddy>),
# so the header-only vcpkg port cannot replace it.
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

# KArchive: vcpkg only ships a Qt5 build (kf5archive) which conflicts with the
# Qt6 application, so keep the standalone fork on CPM. The fork predates Qt 6.9,
# where QString::arg() no longer accepts a QIODevice::OpenMode implicitly; apply
# a small, idempotent source fix after fetching.
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
if(KF5Archive_ADDED)
  file(GLOB_RECURSE _klogg_karchive_sources "${KF5Archive_SOURCE_DIR}/karchive/src/*.cpp")
  foreach(_src ${_klogg_karchive_sources})
    file(READ "${_src}" _contents)
    string(REPLACE ".arg(mode)" ".arg(static_cast<int>(mode))" _patched "${_contents}")
    string(REPLACE ".arg(d->mode)" ".arg(static_cast<int>(d->mode))" _patched "${_patched}")
    if(NOT _patched STREQUAL _contents)
      file(WRITE "${_src}" "${_patched}")
    endif()
  endforeach()
endif()

cpmaddpackage("gh:lemire/streamvbyte@1.0.0")

# KDSingleApplication links Qt, so it is compiled against the application's own
# (system) Qt rather than pulling a second Qt through vcpkg.
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
  KDToolBox
  GITHUB_REPOSITORY
  KDAB/KDToolBox
  GIT_TAG
  6468867d1a46eabe1bcb2cd342f338fe66f06675
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

# ---------------------------------------------------------------------------
# Mark remaining CPM-built dependency includes as system to silence warnings.
# (vcpkg config packages already export their includes as SYSTEM.)
# ---------------------------------------------------------------------------
set(klogg_cpm_targets
    streamvbyte
    kdtoolbox
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
