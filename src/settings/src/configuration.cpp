/*
 * Copyright (C) 2009, 2010, 2013, 2015 Nicolas Bonnefon and other contributors
 *
 * This file is part of glogg.
 *
 * glogg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * glogg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with glogg.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Copyright (C) 2016 -- 2019 Anton Filimonov and other contributors
 *
 * This file is part of klogg.
 *
 * klogg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * klogg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with klogg.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <algorithm>
#include <mutex>

#include <QFontInfo>
#include <QKeySequence>
#include <qcolor.h>
#include <qglobal.h>
#include <qmap.h>
#include <qvariant.h>

#include "configuration.h"
#include "log.h"
#include "shortcuts.h"
#include "styles.h"

namespace {
std::once_flag fontInitFlag;
static const Configuration DefaultConfiguration = {};

} // namespace

Configuration::Configuration()
{
    splitterSizes_ << 400 << 100;
}

// Accessor functions
QFont Configuration::mainFont() const
{
    std::call_once( fontInitFlag, [ this ]() {
        mainFont_.setStyleHint( QFont::Courier, QFont::PreferOutline );

        QFontInfo fi( mainFont_ );
        LOG_INFO << "DefaultConfiguration font is " << fi.family().toStdString() << ": "
                 << fi.pointSize();
    } );

    return mainFont_;
}

void Configuration::setMainFont( QFont newFont )
{
    LOG_DEBUG << "Configuration::setMainFont";

    mainFont_ = newFont;
}

void Configuration::retrieveFromStorage( QSettings& settings )
{
    LOG_DEBUG << "Configuration::retrieveFromStorage";

    // Fonts
    QString family
        = settings.value( "mainFont.family", DefaultConfiguration.mainFont_.family() ).toString();
    int size
        = settings.value( "mainFont.size", DefaultConfiguration.mainFont_.pointSize() ).toInt();

    // If no config read, keep the DefaultConfiguration
    if ( !family.isNull() )
        mainFont_ = QFont( family, size );

    forceFontAntialiasing_
        = settings.value( "mainFont.antialiasing", DefaultConfiguration.forceFontAntialiasing_ )
              .toBool();

    language_ = settings.value( "view.language", DefaultConfiguration.language_ ).toString();

    enableQtHighDpi_
        = settings.value( "view.qtHiDpi", DefaultConfiguration.enableQtHighDpi_ ).toBool();

    scaleFactorRounding_
        = settings.value( "view.scaleFactorRounding", DefaultConfiguration.scaleFactorRounding_ )
              .toInt();

    // Regexp types
    mainRegexpType_ = static_cast<SearchRegexpType>(
        settings
            .value( "regexpType.main", static_cast<int>( DefaultConfiguration.mainRegexpType_ ) )
            .toInt() );
    quickfindRegexpType_ = static_cast<SearchRegexpType>(
        settings
            .value( "regexpType.quickfind",
                    static_cast<int>( DefaultConfiguration.quickfindRegexpType_ ) )
            .toInt() );
    regexpEngine_ = static_cast<RegexpEngine>(
        settings
            .value( "regexpType.engine", static_cast<int>( DefaultConfiguration.regexpEngine_ ) )
            .toInt() );
    quickfindIncremental_
        = settings.value( "quickfind.incremental", DefaultConfiguration.quickfindIncremental_ )
              .toBool();

    enableMainSearchHighlight_
        = settings
              .value( "regexpType.mainHighlight", DefaultConfiguration.enableMainSearchHighlight_ )
              .toBool();

    enableMainSearchHighlightVariance_
        = settings
              .value( "regexpType.mainHighlightVariate",
                      DefaultConfiguration.enableMainSearchHighlightVariance_ )
              .toBool();

    mainSearchBackColor_.setNamedColor(
        settings
            .value( "regexpType.mainBackColor",
                    DefaultConfiguration.mainSearchBackColor_.name( QColor::HexArgb ) )
            .toString() );

    qfBackColor_.setNamedColor(
        settings
            .value( "regexpType.quickfindBackColor",
                    DefaultConfiguration.qfBackColor_.name( QColor::HexArgb ) )
            .toString() );

    qfIgnoreCase_
        = settings.value( "quickfind.ignore_case", DefaultConfiguration.qfIgnoreCase_ ).toBool();

    autoRunSearchOnPatternChange_ = settings
                                        .value( "regexpType.autoRunSearch",
                                                DefaultConfiguration.autoRunSearchOnPatternChange_ )
                                        .toBool();

    // "Advanced" settings
    nativeFileWatchEnabled_
        = settings.value( "nativeFileWatch.enabled", DefaultConfiguration.nativeFileWatchEnabled_ )
              .toBool();
    settings.remove( "nativeFileWatch.enabled" );
    nativeFileWatchEnabled_
        = settings.value( "filewatch.useNative", nativeFileWatchEnabled_ ).toBool();

    pollingEnabled_
        = settings.value( "polling.enabled", DefaultConfiguration.pollingEnabled_ ).toBool();
    settings.remove( "polling.enabled" );
    pollingEnabled_ = settings.value( "filewatch.usePolling", pollingEnabled_ ).toBool();

    pollIntervalMs_
        = settings.value( "polling.intervalMs", DefaultConfiguration.pollIntervalMs_ ).toInt();
    settings.remove( "polling.intervalMs" );
    pollIntervalMs_ = settings.value( "filewatch.pollingIntervalMs", pollIntervalMs_ ).toInt();

    fastModificationDetection_ = settings
                                     .value( "filewatch.fastModificationDetection",
                                             DefaultConfiguration.fastModificationDetection_ )
                                     .toBool();

    allowFollowOnScroll_
        = settings
              .value( "filewatch.allowFollowOnScroll", DefaultConfiguration.allowFollowOnScroll_ )
              .toBool();

    loadLastSession_
        = settings.value( "session.loadLast", DefaultConfiguration.loadLastSession_ ).toBool();
    allowMultipleWindows_
        = settings.value( "session.multipleWindows", DefaultConfiguration.allowMultipleWindows_ )
              .toBool();
    followFileOnLoad_
        = settings.value( "session.followOnLoad", DefaultConfiguration.followFileOnLoad_ ).toBool();

    enableLogging_
        = settings.value( "logging.enableLogging", DefaultConfiguration.enableLogging_ ).toBool();
    loggingLevel_
        = settings.value( "logging.verbosity", DefaultConfiguration.loggingLevel_ ).toInt();

    enableVersionChecking_
        = settings.value( "versionchecker.enabled", DefaultConfiguration.enableVersionChecking_ )
              .toBool();

    extractArchives_
        = settings.value( "archives.extract", DefaultConfiguration.extractArchives_ ).toBool();
    extractArchivesAlways_
        = settings.value( "archives.extractAlways", DefaultConfiguration.extractArchivesAlways_ )
              .toBool();

    // "Perf" settings
    useParallelSearch_
        = settings.value( "perf.useParallelSearch", DefaultConfiguration.useParallelSearch_ )
              .toBool();
    useSearchResultsCache_
        = settings
              .value( "perf.useSearchResultsCache", DefaultConfiguration.useSearchResultsCache_ )
              .toBool();
    searchResultsCacheLines_ = settings
                                   .value( "perf.searchResultsCacheLines",
                                           DefaultConfiguration.searchResultsCacheLines_ )
                                   .toUInt();
    indexReadBufferSizeMb_
        = settings
              .value( "perf.indexReadBufferSizeMb", DefaultConfiguration.indexReadBufferSizeMb_ )
              .toInt();
    searchReadBufferSizeLines_ = settings
                                     .value( "perf.searchReadBufferSizeLines",
                                             DefaultConfiguration.searchReadBufferSizeLines_ )
                                     .toInt();
    searchThreadPoolSize_
        = settings.value( "perf.searchThreadPoolSize", DefaultConfiguration.searchThreadPoolSize_ )
              .toInt();
    keepFileClosed_
        = settings.value( "perf.keepFileClosed", DefaultConfiguration.keepFileClosed_ ).toBool();

    optimizeForNotLatinEncodings_ = settings
                                        .value( "perf.optimizeForNotLatinEncodings",
                                                DefaultConfiguration.optimizeForNotLatinEncodings_ )
                                        .toBool();

    verifySslPeers_
        = settings.value( "net.verifySslPeers", DefaultConfiguration.verifySslPeers_ ).toBool();

    // View settings
    overviewVisible_
        = settings.value( "view.overviewVisible", DefaultConfiguration.overviewVisible_ ).toBool();
    lineNumbersVisibleInMain_ = settings
                                    .value( "view.lineNumbersVisibleInMain",
                                            DefaultConfiguration.lineNumbersVisibleInMain_ )
                                    .toBool();
    lineNumbersVisibleInFiltered_ = settings
                                        .value( "view.lineNumbersVisibleInFiltered",
                                                DefaultConfiguration.lineNumbersVisibleInFiltered_ )
                                        .toBool();
    minimizeToTray_
        = settings.value( "view.minimizeToTray", DefaultConfiguration.minimizeToTray_ ).toBool();

    hideAnsiColorSequences_
        = settings
              .value( "view.hideAnsiColorSequences", DefaultConfiguration.hideAnsiColorSequences_ )
              .toBool();

    style_ = settings.value( "view.style", DefaultConfiguration.style_ ).toString();

    auto styles = StyleManager::availableStyles();
    if ( !styles.contains( style_ ) ) {
        style_ = StyleManager::defaultPlatformStyle();
    }
    if ( !styles.contains( style_ ) ) {
        style_ = styles.front();
    }

    // DefaultConfiguration crawler settings
    searchAutoRefresh_
        = settings.value( "defaultView.searchAutoRefresh", DefaultConfiguration.searchAutoRefresh_ )
              .toBool();
    searchIgnoreCase_
        = settings.value( "defaultView.searchIgnoreCase", DefaultConfiguration.searchIgnoreCase_ )
              .toBool();

    defaultEncodingMib_
        = settings.value( "defaultView.encodingMib", DefaultConfiguration.defaultEncodingMib_ )
              .toInt();

    if ( settings.contains( "defaultView.splitterSizes" ) ) {
        splitterSizes_.clear();

        const auto sizes = settings.value( "defaultView.splitterSizes" ).toList();
        std::transform( sizes.cbegin(), sizes.cend(), std::back_inserter( splitterSizes_ ),
                        []( auto v ) { return v.toInt(); } );
    }

    if ( settings.contains( "shortcuts.mapping" ) ) {
        shortcuts_.clear();

        const auto mapping = settings.value( "shortcuts.mapping" ).toMap();
        for ( auto keys = mapping.begin(); keys != mapping.end(); ++keys ) {
            shortcuts_.emplace( keys.key().toStdString(), keys.value().toStringList() );
        }
        settings.remove( "shortcuts.mapping" );
    }

    const auto shortcutsCount = settings.beginReadArray( "shortcuts" );
    for ( auto shortcutIndex = 0; shortcutIndex < shortcutsCount; ++shortcutIndex ) {
        settings.setArrayIndex( static_cast<int>( shortcutIndex ) );
        const auto action = settings.value( "action", "" ).toString();
        if ( !action.isEmpty() ) {
            const auto keys = settings.value( "keys", QStringList() ).toStringList();
            shortcuts_.emplace( action.toStdString(), keys );
        }
    }
    settings.endArray();

    settings.beginGroup( "dark" );
    for ( auto& color : darkPalette_ ) {
        color.second = settings.value( color.first, color.second ).toString();
    }
    settings.endGroup();
}

void Configuration::saveToStorage( QSettings& settings ) const
{
    LOG_DEBUG << "Configuration::saveToStorage";

    QFontInfo fi( mainFont_ );

    settings.setValue( "mainFont.family", fi.family() );
    settings.setValue( "mainFont.size", fi.pointSize() );
    settings.setValue( "mainFont.antialiasing", forceFontAntialiasing_ );
    settings.setValue( "view.language", language_ );

    settings.setValue( "regexpType.engine", static_cast<int>( regexpEngine_ ) );

    settings.setValue( "regexpType.main", static_cast<int>( mainRegexpType_ ) );
    settings.setValue( "regexpType.mainBackColor", mainSearchBackColor_.name( QColor::HexArgb ) );
    settings.setValue( "regexpType.mainHighlight", enableMainSearchHighlight_ );
    settings.setValue( "regexpType.mainHighlightVariate", enableMainSearchHighlightVariance_ );
    settings.setValue( "regexpType.autoRunSearch", autoRunSearchOnPatternChange_ );

    settings.setValue( "regexpType.quickfind", static_cast<int>( quickfindRegexpType_ ) );
    settings.setValue( "regexpType.quickfindBackColor", qfBackColor_.name( QColor::HexArgb ) );

    settings.setValue( "quickfind.incremental", quickfindIncremental_ );
    settings.setValue( "quickfind.ignore_case", qfIgnoreCase_ );

    settings.setValue( "filewatch.useNative", nativeFileWatchEnabled_ );
    settings.setValue( "filewatch.usePolling", pollingEnabled_ );
    settings.setValue( "filewatch.pollingIntervalMs", pollIntervalMs_ );
    settings.setValue( "filewatch.fastModificationDetection", fastModificationDetection_ );
    settings.setValue( "filewatch.allowFollowOnScroll", allowFollowOnScroll_ );

    settings.setValue( "session.loadLast", loadLastSession_ );
    settings.setValue( "session.multipleWindows", allowMultipleWindows_ );
    settings.setValue( "session.followOnLoad", followFileOnLoad_ );

    settings.setValue( "logging.enableLogging", enableLogging_ );
    settings.setValue( "logging.verbosity", loggingLevel_ );

    settings.setValue( "versionchecker.enabled", enableVersionChecking_ );

    settings.setValue( "archives.extract", extractArchives_ );
    settings.setValue( "archives.extractAlways", extractArchivesAlways_ );

    settings.setValue( "perf.useParallelSearch", useParallelSearch_ );
    settings.setValue( "perf.useSearchResultsCache", useSearchResultsCache_ );
    settings.setValue( "perf.searchResultsCacheLines", searchResultsCacheLines_ );
    settings.setValue( "perf.indexReadBufferSizeMb", indexReadBufferSizeMb_ );
    settings.setValue( "perf.searchReadBufferSizeLines", searchReadBufferSizeLines_ );
    settings.setValue( "perf.searchThreadPoolSize", searchThreadPoolSize_ );
    settings.setValue( "perf.keepFileClosed", keepFileClosed_ );
    settings.setValue( "perf.optimizeForNotLatinEncodings", optimizeForNotLatinEncodings_ );

    settings.setValue( "net.verifySslPeers", verifySslPeers_ );

    settings.setValue( "view.overviewVisible", overviewVisible_ );
    settings.setValue( "view.lineNumbersVisibleInMain", lineNumbersVisibleInMain_ );
    settings.setValue( "view.lineNumbersVisibleInFiltered", lineNumbersVisibleInFiltered_ );
    settings.setValue( "view.minimizeToTray", minimizeToTray_ );
    settings.setValue( "view.style", style_ );

    settings.setValue( "view.qtHiDpi", enableQtHighDpi_ );
    settings.setValue( "view.scaleFactorRounding", scaleFactorRounding_ );

    settings.setValue( "view.hideAnsiColorSequences", hideAnsiColorSequences_ );

    settings.setValue( "defaultView.searchAutoRefresh", searchAutoRefresh_ );
    settings.setValue( "defaultView.searchIgnoreCase", searchIgnoreCase_ );
    settings.setValue( "defaultView.encodingMib", defaultEncodingMib_ );

    QList<QVariant> splitterSizes;
    std::transform( splitterSizes_.cbegin(), splitterSizes_.cend(),
                    std::back_inserter( splitterSizes ),
                    []( auto s ) { return QVariant::fromValue( s ); } );

    settings.setValue( "defaultView.splitterSizes", splitterSizes );

    settings.beginWriteArray( "shortcuts" );
    auto shortcutIndex = 0;
    for ( const auto& mapping : shortcuts_ ) {
        settings.setArrayIndex( shortcutIndex );
        settings.setValue( "action", QString::fromStdString( mapping.first ) );
        settings.setValue( "keys", mapping.second );
        shortcutIndex++;
    }
    settings.endArray();

    settings.beginGroup( "dark" );
    for ( const auto& color : darkPalette_ ) {
        settings.setValue( color.first, color.second );
    }
    settings.endGroup();
}
