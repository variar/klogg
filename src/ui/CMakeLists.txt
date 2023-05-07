add_library(
  klogg_ui STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include/abstractlogview.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/crawlerwidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/filteredview.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/highlightersdialog.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/highlighteredit.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/highlightersetedit.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/highlighterset.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/highlightersmenu.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/highlightedmatch.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/predefinedfilters.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/predefinedfilterscombobox.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/predefinedfiltersdialog.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/infoline.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/pathline.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/logmainview.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/mainwindow.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/mainwindowtext.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/menuactiontooltipbehavior.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/optionsdialog.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/overview.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/overviewwidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/qfnotifications.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/quickfind.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/quickfindmux.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/quickfindpattern.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/quickfindwidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/recentfiles.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/savedsearches.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/selection.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/session.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/sessioninfo.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/signalmux.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/tabbedcrawlerwidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/viewinterface.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/viewtools.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/scratchpad.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/tabbedscratchpad.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/encodings.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/favoritefiles.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/tabnamemapping.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/iconloader.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/displayfilepath.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/downloader.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/decompressor.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/fontutils.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/colorlabelsmanager.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/highlighteredit.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/include/highlightersetedit.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/include/highlightersdialog.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/include/predefinedfiltersdialog.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/include/optionsdialog.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/src/abstractlogview.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/crawlerwidget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/filteredview.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/highlightersdialog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/highlighteredit.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/highlightersetedit.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/highlighterset.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/predefinedfilters.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/predefinedfilterscombobox.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/predefinedfiltersdialog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/infoline.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/pathline.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/logmainview.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/mainwindow.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/mainwindowtext.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/menuactiontooltipbehavior.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/optionsdialog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/overview.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/overviewwidget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/quickfind.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/quickfindmux.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/quickfindpattern.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/quickfindwidget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/recentfiles.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/savedsearches.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/selection.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/session.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/sessioninfo.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/signalmux.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/tabbedcrawlerwidget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/viewtools.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/scratchpad.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/tabbedscratchpad.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/favoritefiles.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/tabnamemapping.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/iconloader.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/displayfilepath.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/downloader.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/decompressor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/colorlabelsmanager.cpp
)

set_target_properties(klogg_ui PROPERTIES AUTOUIC ON)
set_target_properties(klogg_ui PROPERTIES AUTOMOC ON)

set_property(
  TARGET klogg_ui
  APPEND
  PROPERTY AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/ui"
)

if(KLOGG_USE_LTO)
  set_property(TARGET klogg_ui PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

target_include_directories(klogg_ui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(
  klogg_ui
  PUBLIC project_options
         project_warnings
         klogg_logdata
         klogg_versioncheck
         klogg_karchive
         Qt${QT_VERSION_MAJOR}::Widgets
         Qt${QT_VERSION_MAJOR}::Xml
)

if(WIN32)
  target_link_libraries(klogg_ui PUBLIC user32)
endif()
