add_library(ui STATIC
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
        ${CMAKE_CURRENT_SOURCE_DIR}/include/infoline.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/pathline.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/logmainview.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/mainwindow.h
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

        ${CMAKE_CURRENT_SOURCE_DIR}/include/highlighteredit.ui
        ${CMAKE_CURRENT_SOURCE_DIR}/include/highlightersetedit.ui
        ${CMAKE_CURRENT_SOURCE_DIR}/include/highlightersdialog.ui
        ${CMAKE_CURRENT_SOURCE_DIR}/include/optionsdialog.ui

        ${CMAKE_CURRENT_SOURCE_DIR}/src/abstractlogview.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/crawlerwidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/filteredview.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/highlightersdialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/highlighteredit.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/highlightersetedit.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/highlighterset.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/predefinedfilters.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/infoline.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/pathline.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/logmainview.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/mainwindow.cpp
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
)

set_target_properties(ui PROPERTIES AUTOUIC ON)
set_target_properties(ui PROPERTIES AUTOMOC ON)

set_property(TARGET ui APPEND PROPERTY AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/ui")
if (${USE_LTO})
set_property(TARGET ui PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

target_include_directories(ui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(ui PUBLIC Qt5::Widgets Qt5::Xml logdata versioncheck absl::any)

if (WIN32)
    target_link_libraries(ui PUBLIC user32)
endif()
