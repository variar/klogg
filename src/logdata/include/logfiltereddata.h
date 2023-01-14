/*
 * Copyright (C) 2009, 2010, 2011, 2012, 2017 Nicolas Bonnefon and other contributors
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

#ifndef LOGFILTEREDDATA_H
#define LOGFILTEREDDATA_H

#include <cstdint>
#include <functional>
#include <memory>
#include <tuple>
#include <unordered_map>

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QStringList>

#include <KDSignalThrottler.h>

#include "abstractlogdata.h"
#include "hsregularexpression.h"
#include "linetypes.h"
#include "logfiltereddataworker.h"
#include "synchronization.h"

class LogData;
class QTimer;

// A list of matches found in a LogData, it stores all the matching lines,
// which can be accessed using the AbstractLogData interface, together with
// the original line number where they were found.
// Constructing such objet does not start the search.
// This object should be constructed by a LogData.
class LogFilteredData : public AbstractLogData {
    Q_OBJECT

  public:
    // Constructor used by LogData
    explicit LogFilteredData( const LogData* logData );

    // Starts the async search, sending newDataAvailable() when new data found.
    // If a search is already in progress this function will block until
    // it is done, so the application should call interruptSearch() first.
    void runSearch( const RegularExpressionPattern& regExp, LineNumber startLine,
                    LineNumber endLine );
    // Shortcut for runSearch on all file
    void runSearch( const RegularExpressionPattern& regExp );

    // Add to the existing search, starting at the line when the search was
    // last stopped. Used when the file on disk has been added too.
    void updateSearch( LineNumber startLine, LineNumber endLine );
    // Interrupt the running search if one is in progress.
    // Nothing is done if no search is in progress.
    void interruptSearch();
    // Clear the search and the list of results.
    void clearSearch( bool dropCache = false );

    // Returns the line number in the original LogData where the element
    // 'index' was found.
    LineNumber getMatchingLineNumber( LineNumber index ) const;
    // Returns the line 'index' in filterd log data that matches
    // given original line number
    LineNumber getLineIndexNumber( LineNumber lineNumber ) const;

    // Returns the number of lines in the source log data
    LinesCount getNbTotalLines() const;
    // Returns the number of matches (independently of the visibility)
    LinesCount getNbMatches() const;
    // Returns the number of marks (independently of the visibility)
    LinesCount getNbMarks() const;

    LineType lineTypeByIndex( LineNumber index ) const;
    LineType lineTypeByLine( LineNumber lineNumber ) const;

    // Marks interface (delegated to a Marks object)

    // Add a mark at the given line
    void addMark( LineNumber line );
    // Get the first mark after the line passed
    OptionalLineNumber getMarkAfter( LineNumber line ) const;
    // Get the first mark before the line passed
    OptionalLineNumber getMarkBefore( LineNumber line ) const;
    // Delete the mark present on the passed line
    void deleteMark( LineNumber line );
    // Toggle presence of the mark on the passed line.
    void toggleMark( LineNumber line );
    // Completely clear the marks list.
    void clearMarks();
    // Get all marked lines
    QList<LineNumber> getMarks() const;

    // Changes what the AbstractLogData returns via its getXLines/getNbLines
    // API.
    enum class VisibilityFlags {
        None = static_cast<LineType::Int>( LineTypeFlags::Plain ), // this is for internal use
        Matches = static_cast<LineType::Int>( LineTypeFlags::Match ),
        Marks = static_cast<LineType::Int>( LineTypeFlags::Mark ),
    };
    Q_ENUM( VisibilityFlags );
    Q_DECLARE_FLAGS( Visibility, VisibilityFlags )
    void setVisibility( Visibility visibility );

    void iterateOverLines( const std::function<void( LineNumber )>& callback ) const;
  Q_SIGNALS:
    // Sent when the search has progressed, give the number of matches (so far)
    // and the percentage of completion
    void searchProgressed( LinesCount nbMatches, int progress, LineNumber initialLine );
    void searchProgressedThrottled();

  private Q_SLOTS:
    void handleSearchProgressed( LinesCount nbMatches, int progress, LineNumber initialLine );
    void handleSearchProgressedThrottled();

  private:
    // Implementation of virtual functions
    QString doGetLineString( LineNumber line ) const override;
    QString doGetExpandedLineString( LineNumber line ) const override;
    std::vector<QString> doGetLines( LineNumber first, LinesCount number ) const override;
    std::vector<QString> doGetExpandedLines( LineNumber first, LinesCount number ) const override;
    std::vector<QString> doGetLines( LineNumber first, LinesCount number,
                                     const std::function<QString( LineNumber )>& lineGetter ) const;
    LineNumber doGetLineNumber( LineNumber index ) const override;
    LinesCount doGetNbLine() const override;
    LineLength doGetMaxLength() const override;
    LineLength doGetLineLength( LineNumber line ) const override;

    void doSetDisplayEncoding( const char* encoding ) override;
    QTextCodec* doGetDisplayEncoding() const override;

    void doAttachReader() const override;
    void doDetachReader() const override;

    // Insert new mark into filteredItemsCache_.
    void updateCacheWithMark( uint32_t index, LineNumber line );

    // Returns whether the line number passed is in our list of matching ones.
    bool isLineMatched( LineNumber lineNumber ) const;
    // Returns wheither the passed line has a mark on it.
    bool isLineMarked( LineNumber line ) const;

    // List of the matching line numbers
    SearchResultArray matching_lines_;
    SearchResultArray marks_;
    SearchResultArray marks_and_matches_;

    const LogData* sourceLogData_;

    RegularExpressionPattern currentRegExp_;
    LineLength maxLength_;
    LineLength maxLengthMarks_;
    // Number of lines of the LogData that has been searched for:
    LinesCount nbLinesProcessed_;

    Visibility visibility_;

    LogFilteredDataWorker workerThread_;

    Mutex searchProgressMutex_;
    std::tuple<LinesCount, int, LineNumber> searchProgress_;

    KDToolBox::KDSignalThrottler searchProgressThrottler_;

  private:
    struct CachedSearchResult {
        SearchResultArray matching_lines;
        LineLength maxLength;
    };

    using SearchCacheKey = std::tuple<RegularExpressionPattern, LineNumber::UnderlyingType,
                                      LineNumber::UnderlyingType>;
    struct SearchCacheKeyHash {
        template <class T>
        void hash_combine( std::size_t& seed, const T& v ) const
        {
            seed ^= std::hash<T>()( v ) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 );
        }
        std::size_t operator()( const SearchCacheKey& k ) const
        {
            size_t seed = qHash( std::get<0>( k ).pattern );

            hash_combine( seed, std::get<0>( k ).isPlainText );
            hash_combine( seed, std::get<0>( k ).isBoolean );
            hash_combine( seed, std::get<0>( k ).isCaseSensitive );
            hash_combine( seed, std::get<0>( k ).isExclude );
            hash_combine( seed, std::get<1>( k ) );
            hash_combine( seed, std::get<2>( k ) );
            return seed;
        }
    };

    std::unordered_map<SearchCacheKey, CachedSearchResult, SearchCacheKeyHash> searchResultsCache_;
    SearchCacheKey currentSearchKey_;

    SearchCacheKey makeCacheKey( const RegularExpressionPattern& regExp, LineNumber startLine,
                                 LineNumber endLine )
    {
        return std::make_tuple( regExp, startLine.get(), endLine.get() );
    }

    void updateSearchResultsCache();

    inline LineNumber getExpectedSearchEnd( const SearchCacheKey& cacheKey ) const
    {
        return LineNumber( std::get<2>( cacheKey ) );
    }

    // Utility functions
    const SearchResultArray& currentResultArray() const;
    LineNumber findLogDataLine( LineNumber lineNum ) const;
    LineNumber findFilteredLine( LineNumber lineNum ) const;

    // update maxLengthMarks_ when a Marks was changed.
    void updateMaxLengthMarks( OptionalLineNumber added_line, OptionalLineNumber removed_line );
};

Q_DECLARE_OPERATORS_FOR_FLAGS( LogFilteredData::Visibility )
Q_DECLARE_METATYPE( LogFilteredData::Visibility )

#endif
