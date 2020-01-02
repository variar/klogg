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

#include <QFontInfo>

#include "log.h"

#include "configuration.h"

Configuration::Configuration()
{
    mainFont_.setStyleHint( QFont::Courier, QFont::PreferOutline );

    QFontInfo fi( mainFont_ );
    LOG( logDEBUG ) << "Default font is " << fi.family().toStdString();

    splitterSizes_ << 400 << 100;
}

// Accessor functions
QFont Configuration::mainFont() const
{
    return mainFont_;
}

void Configuration::setMainFont( QFont newFont )
{
    LOG( logDEBUG ) << "Configuration::setMainFont";

    mainFont_ = newFont;
}

void Configuration::retrieveFromStorage( QSettings& settings )
{
    LOG( logDEBUG ) << "Configuration::retrieveFromStorage";

    // Fonts
    QString family = settings.value( "mainFont.family" ).toString();
    int size = settings.value( "mainFont.size" ).toInt();

    // If no config read, keep the default
    if ( !family.isNull() )
        mainFont_ = QFont( family, size );

    // Regexp types
    mainRegexpType_ = static_cast<SearchRegexpType>(
        settings.value( "regexpType.main", mainRegexpType_ ).toInt() );
    quickfindRegexpType_ = static_cast<SearchRegexpType>(
        settings.value( "regexpType.quickfind", quickfindRegexpType_ ).toInt() );
    if ( settings.contains( "quickfind.incremental" ) )
        quickfindIncremental_ = settings.value( "quickfind.incremental" ).toBool();

    // "Advanced" settings
    if ( settings.contains( "nativeFileWatch.enabled" ) )
        nativeFileWatchEnabled_ = settings.value( "nativeFileWatch.enabled" ).toBool();
    if ( settings.contains( "polling.enabled" ) )
        pollingEnabled_ = settings.value( "polling.enabled" ).toBool();
    if ( settings.contains( "polling.intervalMs" ) )
        pollIntervalMs_ = settings.value( "polling.intervalMs" ).toInt();

    if ( settings.contains( "session.loadLast" ) )
        loadLastSession_ = settings.value( "session.loadLast" ).toBool();
    if ( settings.contains( "session.multipleWindows" ) )
        allowMultipleWindows_ = settings.value( "session.multipleWindows" ).toBool();
    if ( settings.contains( "session.followOnLoad" ) )
        followFileOnLoad_ = settings.value( "session.followOnLoad" ).toBool();

    if ( settings.contains( "logging.enableLogging" ) )
        enableLogging_
            = settings.value( "logging.enableLogging" ).toBool();
    if ( settings.contains( "logging.verbosity" ) )
        loggingLevel_
            = static_cast<uint8_t>( settings.value( "logging.verbosity" ).toUInt() );

    if ( settings.contains( "versionchecker.enabled" ) )
        enableVersionChecking_ = settings.value( "versionchecker.enabled" ).toBool();

    if ( settings.contains( "archives.extract" ) )
        extractArchives_ = settings.value( "archives.extract" ).toBool();

    if ( settings.contains( "archives.extractAlways" ) )
        extractArchivesAlways_ = settings.value( "archives.extractAlways" ).toBool();

    // "Perf" settings
    if ( settings.contains( "perf.useParallelSearch" ) )
        useParallelSearch_ = settings.value( "perf.useParallelSearch" ).toBool();
    if ( settings.contains( "perf.useSearchResultsCache" ) )
        useSearchResultsCache_ = settings.value( "perf.useSearchResultsCache" ).toBool();
    if ( settings.contains( "perf.searchResultsCacheLines" ) )
        searchResultsCacheLines_ = settings.value( "perf.searchResultsCacheLines" ).toUInt();
    if ( settings.contains( "perf.indexReadBufferSizeMb" ) )
        indexReadBufferSizeMb_ = settings.value( "perf.indexReadBufferSizeMb" ).toUInt();
    if ( settings.contains( "perf.searchReadBufferSizeLines" ) )
        searchReadBufferSizeLines_ = settings.value( "perf.searchReadBufferSizeLines" ).toUInt();
    if ( settings.contains( "perf.searchThreadPoolSize" ) )
        searchThreadPoolSize_ = settings.value( "perf.searchThreadPoolSize" ).toUInt();
    if ( settings.contains( "perf.keepFileClosed" ) )
        keepFileClosed_ = settings.value( "perf.keepFileClosed" ).toBool();
    if ( settings.contains( "perf.useLineEndingCache" ) )
        useLineEndingCache_ = settings.value( "perf.useLineEndingCache" ).toBool();

    // View settings
    if ( settings.contains( "view.overviewVisible" ) )
        overviewVisible_ = settings.value( "view.overviewVisible" ).toBool();
    if ( settings.contains( "view.lineNumbersVisibleInMain" ) )
        lineNumbersVisibleInMain_ = settings.value( "view.lineNumbersVisibleInMain" ).toBool();
    if ( settings.contains( "view.lineNumbersVisibleInFiltered" ) )
        lineNumbersVisibleInFiltered_
            = settings.value( "view.lineNumbersVisibleInFiltered" ).toBool();
    if ( settings.contains( "view.minimizeToTray" ) )
        minimizeToTray_
            = settings.value( "view.minimizeToTray" ).toBool();

    // Some sanity check (mainly for people upgrading)
    if ( quickfindIncremental_ )
        quickfindRegexpType_ = FixedString;

    // Default crawler settings
    if ( settings.contains( "defaultView.searchAutoRefresh" ) )
        searchAutoRefresh_ = settings.value( "defaultView.searchAutoRefresh" ).toBool();
    if ( settings.contains( "defaultView.searchIgnoreCase" ) )
        searchIgnoreCase_ = settings.value( "defaultView.searchIgnoreCase" ).toBool();
    if ( settings.contains( "defaultView.splitterSizes" ) ) {
        splitterSizes_.clear();

        const auto sizes = settings.value( "defaultView.splitterSizes" ).toList();
        std::transform( sizes.begin(), sizes.end(), std::back_inserter( splitterSizes_ ),
                        []( auto v ) { return v.toInt(); } );
    }
}

void Configuration::saveToStorage( QSettings& settings ) const
{
    LOG( logDEBUG ) << "Configuration::saveToStorage";

    QFontInfo fi( mainFont_ );

    settings.setValue( "mainFont.family", fi.family() );
    settings.setValue( "mainFont.size", fi.pointSize() );
    settings.setValue( "regexpType.main", static_cast<int>( mainRegexpType_ ) );
    settings.setValue( "regexpType.quickfind", static_cast<int>( quickfindRegexpType_ ) );
    settings.setValue( "quickfind.incremental", quickfindIncremental_ );
    settings.setValue( "nativeFileWatch.enabled", nativeFileWatchEnabled_ );
    settings.setValue( "polling.enabled", pollingEnabled_ );
    settings.setValue( "polling.intervalMs", pollIntervalMs_ );
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
    settings.setValue( "perf.useLineEndingCache", useLineEndingCache_ );

    settings.setValue( "view.overviewVisible", overviewVisible_ );
    settings.setValue( "view.lineNumbersVisibleInMain", lineNumbersVisibleInMain_ );
    settings.setValue( "view.lineNumbersVisibleInFiltered", lineNumbersVisibleInFiltered_ );
    settings.setValue( "view.minimizeToTray", minimizeToTray_ );

    settings.setValue( "defaultView.searchAutoRefresh", searchAutoRefresh_ );
    settings.setValue( "defaultView.searchIgnoreCase", searchIgnoreCase_ );

    QList<QVariant> splitterSizes;
    std::transform( splitterSizes_.begin(), splitterSizes_.end(),
                    std::back_inserter( splitterSizes ),
                    []( auto s ) { return QVariant::fromValue( s ); } );

    settings.setValue( "defaultView.splitterSizes", splitterSizes );
}
