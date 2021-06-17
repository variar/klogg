/*
 * Copyright (C) 2009, 2010, 2011, 2013, 2015 Nicolas Bonnefon and other
 * contributors
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

#ifndef KLOGG_CONFIGURATION_H
#define KLOGG_CONFIGURATION_H

#include <QColor>
#include <QFont>
#include <QSettings>
#include <qcolor.h>
#include <string_view>

#include "persistable.h"

// Type of regexp to use for searches
enum class SearchRegexpType {
    ExtendedRegexp,
    Wildcard,
    FixedString,
};

enum class RegexpEngine { Hyperscan, QRegularExpression };

// Configuration class containing everything in the "Settings" dialog
class Configuration final : public Persistable<Configuration> {
  public:
    static const char* persistableName()
    {
        return "Configuration";
    }
    Configuration();

    // Accesses the main font used for display
    QFont mainFont() const;
    void setMainFont( QFont newFont );

    // Accesses the regexp types
    SearchRegexpType mainRegexpType() const
    {
        return mainRegexpType_;
    }
    SearchRegexpType quickfindRegexpType() const
    {
        return quickfindRegexpType_;
    }
    bool isQuickfindIncremental() const
    {
        return quickfindIncremental_;
    }
    void setMainRegexpType( SearchRegexpType type )
    {
        mainRegexpType_ = type;
    }
    void setQuickfindRegexpType( SearchRegexpType type )
    {
        quickfindRegexpType_ = type;
    }
    void setQuickfindIncremental( bool isIncremental )
    {
        quickfindIncremental_ = isIncremental;
    }

    // "Advanced" settings
    bool anyFileWatchEnabled() const
    {
        return nativeFileWatchEnabled() || pollingEnabled();
    }

    bool nativeFileWatchEnabled() const
    {
        return nativeFileWatchEnabled_;
    }
    void setNativeFileWatchEnabled( bool enabled )
    {
        nativeFileWatchEnabled_ = enabled;
    }
    bool pollingEnabled() const
    {
        return pollingEnabled_;
    }
    void setPollingEnabled( bool enabled )
    {
        pollingEnabled_ = enabled;
    }
    int pollIntervalMs() const
    {
        return pollIntervalMs_;
    }
    void setPollIntervalMs( int interval )
    {
        pollIntervalMs_ = interval;
    }

    bool fastModificationDetection() const
    {
        return fastModificationDetection_;
    }

    void setFastModificationDetection( bool fastDetection )
    {
        fastModificationDetection_ = fastDetection;
    }

    bool loadLastSession() const
    {
        return loadLastSession_;
    }
    void setLoadLastSession( bool enabled )
    {
        loadLastSession_ = enabled;
    }
    bool followFileOnLoad() const
    {
        return followFileOnLoad_;
    }
    void setFollowFileOnLoad( bool enabled )
    {
        followFileOnLoad_ = enabled;
    }
    bool allowMultipleWindows() const
    {
        return allowMultipleWindows_;
    }
    void setAllowMultipleWindows( bool enabled )
    {
        allowMultipleWindows_ = enabled;
    }

    // perf settings
    bool useParallelSearch() const
    {
        return useParallelSearch_;
    }
    void setUseParallelSearch( bool enabled )
    {
        useParallelSearch_ = enabled;
    }
    bool useSearchResultsCache() const
    {
        return useSearchResultsCache_;
    }
    void setUseSearchResultsCache( bool enabled )
    {
        useSearchResultsCache_ = enabled;
    }
    unsigned searchResultsCacheLines() const
    {
        return searchResultsCacheLines_;
    }
    void setSearchResultsCacheLines( unsigned lines )
    {
        searchResultsCacheLines_ = lines;
    }
    int indexReadBufferSizeMb() const
    {
        return indexReadBufferSizeMb_;
    }
    void setIndexReadBufferSizeMb( int bufferSizeMb )
    {
        indexReadBufferSizeMb_ = bufferSizeMb;
    }
    int searchReadBufferSizeLines() const
    {
        return searchReadBufferSizeLines_;
    }
    void setSearchReadBufferSizeLines( int lines )
    {
        searchReadBufferSizeLines_ = lines;
    }
    int searchThreadPoolSize() const
    {
        return searchThreadPoolSize_;
    }
    void setSearchThreadPoolSize( int threads )
    {
        searchThreadPoolSize_ = threads;
    }
    bool keepFileClosed() const
    {
        return keepFileClosed_;
    }
    void setKeepFileClosed( bool shouldKeepClosed )
    {
        keepFileClosed_ = shouldKeepClosed;
    }

    RegexpEngine regexpEngine() const
    {
        return regexpEngine_;
    }

    void setRegexpEnging( RegexpEngine engine )
    {
        regexpEngine_ = engine;
    }

    // Accessors
    bool versionCheckingEnabled() const
    {
        return enableVersionChecking_;
    }
    void setVersionCheckingEnabled( bool enabled )
    {
        enableVersionChecking_ = enabled;
    }

    // View settings
    bool isOverviewVisible() const
    {
        return overviewVisible_;
    }
    void setOverviewVisible( bool isVisible )
    {
        overviewVisible_ = isVisible;
    }
    bool mainLineNumbersVisible() const
    {
        return lineNumbersVisibleInMain_;
    }
    bool filteredLineNumbersVisible() const
    {
        return lineNumbersVisibleInFiltered_;
    }
    bool minimizeToTray() const
    {
        return minimizeToTray_;
    }
    QString style() const
    {
        return style_;
    }
    void setMainLineNumbersVisible( bool lineNumbersVisible )
    {
        lineNumbersVisibleInMain_ = lineNumbersVisible;
    }
    void setFilteredLineNumbersVisible( bool lineNumbersVisible )
    {
        lineNumbersVisibleInFiltered_ = lineNumbersVisible;
    }
    void setMinimizeToTray( bool minimizeToTray )
    {
        minimizeToTray_ = minimizeToTray;
    }
    void setStyle( const QString& style )
    {
        style_ = style;
    }

    bool enableLogging() const
    {
        return enableLogging_;
    }
    uint8_t loggingLevel() const
    {
        return loggingLevel_;
    }

    void setEnableLogging( bool enableLogging )
    {
        enableLogging_ = enableLogging;
    }
    void setLoggingLevel( uint8_t level )
    {
        loggingLevel_ = level;
    }

    // Default settings for new views
    bool isSearchAutoRefreshDefault() const
    {
        return searchAutoRefresh_;
    }
    void setSearchAutoRefreshDefault( bool autoRefresh )
    {
        searchAutoRefresh_ = autoRefresh;
    }
    bool isSearchIgnoreCaseDefault() const
    {
        return searchIgnoreCase_;
    }
    void setSearchIgnoreCaseDefault( bool ignoreCase )
    {
        searchIgnoreCase_ = ignoreCase;
    }
    QList<int> splitterSizes() const
    {
        return splitterSizes_;
    }
    void setSplitterSizes( QList<int> sizes )
    {
        splitterSizes_ = std::move( sizes );
    }

    bool extractArchives() const
    {
        return extractArchives_;
    }
    void setExtractArchives( bool extract )
    {
        extractArchives_ = extract;
    }

    bool extractArchivesAlways() const
    {
        return extractArchivesAlways_;
    }
    void setExtractArchivesAlways( bool extract )
    {
        extractArchivesAlways_ = extract;
    }

    bool verifySslPeers() const
    {
        return verifySslPeers_;
    }
    void setVerifySslPeers( bool verify )
    {
        verifySslPeers_ = verify;
    }

    bool forceFontAntialiasing() const
    {
        return forceFontAntialiasing_;
    }
    void setForceFontAntialiasing( bool force )
    {
        forceFontAntialiasing_ = force;
    }

    bool enableQtHighDpi() const
    {
        return enableQtHighDpi_;
    }
    void setEnableQtHighDpi( bool enable )
    {
        enableQtHighDpi_ = enable;
    }

    int scaleFactorRounding() const
    {
        return scaleFactorRounding_;
    }
    void setScaleFactorRounding( int rounding )
    {
        scaleFactorRounding_ = rounding;
    }

    bool mainSearchHighlight() const
    {
        return enableMainSearchHighlight_;
    }
    void setEnableMainSearchHighlight( bool enable )
    {
        enableMainSearchHighlight_ = enable;
    }

    bool variateMainSearchHighlight() const
    {
        return enableMainSearchHighlightVariance_;
    }
    void setVariateMainSearchHighlight( bool enable )
    {
        enableMainSearchHighlightVariance_ = enable;
    }

    QColor mainSearchBackColor() const
    {
        return mainSearchBackColor_;
    }
    void setMainSearchBackColor( QColor color )
    {
        mainSearchBackColor_ = color;
    }

    QColor qfBackColor() const
    {
        return qfBackColor_;
    }
    void setQfBackColor( QColor color )
    {
        qfBackColor_ = color;
    }

    std::map<std::string, QStringList> shortcuts() const
    {
        return shortcuts_;
    }
    void setShortcuts(const std::map<std::string, QStringList>& shortcuts)
    {
        shortcuts_ = shortcuts;
    }

    // Reads/writes the current config in the QSettings object passed
    void saveToStorage( QSettings& settings ) const;
    void retrieveFromStorage( QSettings& settings );

  private:
    // Configuration settings
    mutable QFont mainFont_ = { "DejaVu Sans Mono", 10 };
    SearchRegexpType mainRegexpType_ = SearchRegexpType::ExtendedRegexp;
    SearchRegexpType quickfindRegexpType_ = SearchRegexpType::FixedString;
    bool quickfindIncremental_ = true;

    bool nativeFileWatchEnabled_ = true;
#ifdef Q_OS_WIN
    bool pollingEnabled_ = true;
#else
    bool pollingEnabled_ = false;
#endif

    int pollIntervalMs_ = 2000;

    bool fastModificationDetection_ = false;

    bool loadLastSession_ = true;
    bool followFileOnLoad_ = false;
    bool allowMultipleWindows_ = false;

    // View settings
    bool overviewVisible_ = true;
    bool lineNumbersVisibleInMain_ = false;
    bool lineNumbersVisibleInFiltered_ = true;
    bool minimizeToTray_ = false;
    QString style_;

    // Default settings for new views
    bool searchAutoRefresh_ = false;
    bool searchIgnoreCase_ = false;
    QList<int> splitterSizes_;

    // Performance settings
    bool useSearchResultsCache_ = true;
    unsigned searchResultsCacheLines_ = 1000000;
    bool useParallelSearch_ = true;
    int indexReadBufferSizeMb_ = 16;
    int searchReadBufferSizeLines_ = 5000;
    int searchThreadPoolSize_ = 0;
    bool keepFileClosed_ = false;

    bool enableLogging_ = false;
    uint8_t loggingLevel_ = 4;

    bool enableVersionChecking_ = true;

    bool extractArchives_ = true;
    bool extractArchivesAlways_ = false;

    bool verifySslPeers_ = true;

    bool forceFontAntialiasing_ = false;
    bool enableQtHighDpi_ = true;

    int scaleFactorRounding_ = 1;

    RegexpEngine regexpEngine_ = RegexpEngine::Hyperscan;

    QColor qfBackColor_ = Qt::yellow;
    QColor mainSearchBackColor_ = Qt::lightGray;
    bool enableMainSearchHighlight_ = false;
    bool enableMainSearchHighlightVariance_ = false;

    std::map<std::string, QStringList> shortcuts_;
};

#endif
