/*
 * Copyright (C) 2009, 2010, 2011, 2012, 2013, 2017 Nicolas Bonnefon and other contributors
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

// This file implements LogFilteredData
// It stores a pointer to the LogData that created it,
// so should always be destroyed before the LogData.

#include "log.h"

#include <QString>
#include <cassert>
#include <limits>

#include "logdata.h"
#include "marks.h"
#include "logfiltereddata.h"

#include "persistentinfo.h"
#include "configuration.h"

namespace {

}

// Creates an empty set. It must be possible to display it without error.
// FIXME
LogFilteredData::LogFilteredData() : AbstractLogData(),
    matching_lines_(),
    currentRegExp_(),
    visibility_(),
    filteredItemsCache_(),
    workerThread_( nullptr ),
    marks_()
{
    /* Prevent any more searching */
    maxLength_ = 0_length;
    maxLengthMarks_ = 0_length;
    nbLinesProcessed_ = 0_lcount;
    visibility_ = MarksAndMatches;

    filteredItemsCacheDirty_ = true;
}

// Usual constructor: just copy the data, the search is started by runSearch()
LogFilteredData::LogFilteredData( const LogData* logData )
    : AbstractLogData(),
    matching_lines_( SearchResultArray() ),
    currentRegExp_(),
    visibility_(),
    filteredItemsCache_(),
    workerThread_( logData ),
    marks_()
{
    // Starts with an empty result list
    maxLength_ = 0_length;
    maxLengthMarks_ = 0_length;
    nbLinesProcessed_ = 0_lcount;

    sourceLogData_ = logData;

    visibility_ = MarksAndMatches;

    filteredItemsCacheDirty_ = true;

    // Forward the update signal
    connect( &workerThread_, &LogFilteredDataWorkerThread::searchProgressed,
            this, &LogFilteredData::handleSearchProgressed );

    // Starts the worker thread
    workerThread_.start();
}

LogFilteredData::~LogFilteredData()
{
    // FIXME
    // workerThread_.stop();
}

//
// Public functions
//

void LogFilteredData::runSearch(const QRegularExpression& regExp)
{
    runSearch( regExp, 0_lnum, LineNumber( getNbTotalLines().get() ) );
}

// Run the search and send newDataAvailable() signals.
void LogFilteredData::runSearch(const QRegularExpression& regExp,
                                LineNumber startLine, LineNumber endLine)
{
    LOG(logDEBUG) << "Entering runSearch";

    const auto& config = Persistent<Configuration>( "settings" );


    clearSearch();
    currentRegExp_ = regExp;
    currentSearchKey_ = makeCacheKey( regExp, startLine, endLine );

    bool shouldRunSearch = true;
    if ( config.useSearchResultsCache() ) {
        const auto cachedResults = searchResultsCache_.find( currentSearchKey_ );
        if ( cachedResults != std::end( searchResultsCache_ ) ) {
            LOG(logDEBUG) << "Got result from cache";
            shouldRunSearch = false;
            matching_lines_ = cachedResults.value().matching_lines;
            maxLength_ = cachedResults.value().maxLength;
            emit searchProgressed(
                        LinesCount( static_cast<LinesCount::UnderlyingType>( matching_lines_.size() ) ),
                        100,
                        startLine );
        }
    }

    if ( shouldRunSearch ) {
        workerThread_.search( currentRegExp_, startLine, endLine );
    }
}

void LogFilteredData::updateSearch(LineNumber startLine, LineNumber endLine)
{
    LOG(logDEBUG) << "Entering updateSearch";

    currentSearchKey_ = makeCacheKey( currentRegExp_, startLine, endLine );
    workerThread_.updateSearch( currentRegExp_, startLine, endLine,
                                LineNumber( nbLinesProcessed_.get() ) );
}

void LogFilteredData::interruptSearch()
{
    LOG(logDEBUG) << "Entering interruptSearch";

    workerThread_.interrupt();
}

void LogFilteredData::clearSearch()
{
    currentRegExp_ = QRegularExpression();
    matching_lines_.clear();
    maxLength_        = 0_length;
    nbLinesProcessed_ = 0_lcount;
    filteredItemsCacheDirty_ = true;
}

LineNumber LogFilteredData::getMatchingLineNumber( LineNumber matchNum ) const
{
    return findLogDataLine( matchNum );
}

LineNumber LogFilteredData::getLineIndexNumber( LineNumber lineNumber ) const
{
    return findFilteredLine( lineNumber );
}

// Scan the list for the 'lineNumber' passed
bool LogFilteredData::isLineInMatchingList( LineNumber lineNumber )
{
    uint32_t index;                                    // Not used
    return lookupLineNumber<SearchResultArray>(
            matching_lines_, lineNumber, index);
}


LinesCount LogFilteredData::getNbTotalLines() const
{
    return sourceLogData_->getNbLine();
}

LinesCount LogFilteredData::getNbMatches() const
{
    return LinesCount( static_cast<LinesCount::UnderlyingType>( matching_lines_.size() ) );
}

LinesCount LogFilteredData::getNbMarks() const
{
    return LinesCount(marks_.size());
}

LogFilteredData::FilteredLineType
    LogFilteredData::filteredLineTypeByIndex( LineNumber index ) const
{
    // If we are only showing one type, the line is there because
    // it is of this type.
    if ( visibility_ == MatchesOnly )
        return Match;
    else if ( visibility_ == MarksOnly )
        return Mark;
    else {
        // If it is MarksAndMatches, we have to look.
        // Regenerate the cache if needed
        if ( filteredItemsCacheDirty_ )
            regenerateFilteredItemsCache();

        return filteredItemsCache_[ index.get() ].type();
    }
}

// Delegation to our Marks object

void LogFilteredData::addMark( LineNumber line, QChar mark )
{
    if ( line < sourceLogData_->getNbLine() ) {
        marks_.addMark( line, mark );
        maxLengthMarks_ = qMax( maxLengthMarks_,
                sourceLogData_->getLineLength( line ) );
        filteredItemsCacheDirty_ = true;
    }
    else
        LOG(logERROR) << "LogFilteredData::addMark\
 trying to create a mark outside of the file.";
}

LineNumber LogFilteredData::getMark( QChar mark ) const
{
    return marks_.getMark( mark );
}

bool LogFilteredData::isLineMarked( LineNumber line ) const
{
    return marks_.isLineMarked( line );
}

OptionalLineNumber LogFilteredData::getMarkAfter( LineNumber line ) const
{
    OptionalLineNumber marked_line;

    for ( const auto& mark : marks_ ) {
        if ( mark.lineNumber() > line ) {
            marked_line = mark.lineNumber();
            break;
        }
    }

    return marked_line;
}

OptionalLineNumber LogFilteredData::getMarkBefore( LineNumber line ) const
{
    OptionalLineNumber marked_line;

    for ( const auto& mark : marks_ ) {
        if ( mark.lineNumber() >= line ) {
            break;
        }
        marked_line = mark.lineNumber();
    }

    return marked_line;
}

void LogFilteredData::deleteMark( QChar mark )
{
    marks_.deleteMark( mark );
    filteredItemsCacheDirty_ = true;

    // FIXME: maxLengthMarks_
}

void LogFilteredData::deleteMark( LineNumber line )
{
    marks_.deleteMark( line );
    filteredItemsCacheDirty_ = true;

    // Now update the max length if needed
    if ( sourceLogData_->getLineLength( line ) >= maxLengthMarks_ ) {
        LOG(logDEBUG) << "deleteMark recalculating longest mark";
        maxLengthMarks_ = 0_length;
        for ( const auto& mark : marks_ ) {
            LOG(logDEBUG) << "line " << mark.lineNumber();
            maxLengthMarks_ = qMax( maxLengthMarks_,
                    sourceLogData_->getLineLength( mark.lineNumber() ) );
        }
    }
}

void LogFilteredData::clearMarks()
{
    marks_.clear();
    filteredItemsCacheDirty_ = true;
    maxLengthMarks_ = 0_length;
}

QList<LineNumber> LogFilteredData::getMarks() const
{
    QList<LineNumber> markedLines;
    for ( const auto& m : marks_ ) {
        markedLines.append( m.lineNumber() );
    }
    return markedLines;
}

void LogFilteredData::setVisibility( Visibility visi )
{
    visibility_ = visi;
}

//
// Slots
//
void LogFilteredData::handleSearchProgressed( LinesCount nbMatches, int progress, LineNumber initialLine )
{
    const auto& config = Persistent<Configuration>( "settings" );

    LOG(logDEBUG) << "LogFilteredData::handleSearchProgressed matches="
        << nbMatches << " progress=" << progress;

    workerThread_.getSearchResult( &maxLength_, &matching_lines_, &nbLinesProcessed_ );
    filteredItemsCacheDirty_ = true;

    if ( progress == 100 && config.useSearchResultsCache()
         && nbLinesProcessed_.get() == getExpectedSearchEnd( currentSearchKey_ ).get() ) {

        const auto maxCacheLines = config.searchResultsCacheLines();

        if ( matching_lines_.size() > maxCacheLines ) {
            LOG(logDEBUG) << "LogFilteredData: too many matches to place in cache";
        }
        else if ( !config.useSearchResultsCache() ) {
            LOG(logDEBUG) << "LogFilteredData: search results cache disabled by configs";
        }
        else {
            LOG(logDEBUG) << "LogFilteredData: caching results for pattern "
                          << currentRegExp_.pattern().toStdString();

            searchResultsCache_[ currentSearchKey_ ] = { matching_lines_, maxLength_ };

            auto cacheSize = std::accumulate(
                searchResultsCache_.cbegin(), searchResultsCache_.cend(), std::size_t{},
                []( auto val, const auto& cachedResults )
                {
                    return val + cachedResults.matching_lines.size();
                });

            LOG(logDEBUG) << "LogFilteredData: cache size " << cacheSize;

            auto cachedResult = std::begin( searchResultsCache_ );
            while( cachedResult != std::end( searchResultsCache_ )
                  && cacheSize > maxCacheLines ) {

                if ( cachedResult.key() == currentSearchKey_ ) {
                    ++cachedResult;
                    continue;
                }

                cacheSize -= cachedResult.value().matching_lines.size();
                cachedResult = searchResultsCache_.erase( cachedResult );
            }
        }
    }

    emit searchProgressed( nbMatches, progress, initialLine );
}

LineNumber LogFilteredData::findLogDataLine( LineNumber lineNum ) const
{
    auto line = maxValue<LineNumber>();
    if ( visibility_ == MatchesOnly ) {
        if ( lineNum.get() < matching_lines_.size() ) {
            line = matching_lines_[lineNum.get()].lineNumber();
        }
        else {
            LOG(logERROR) << "Index too big in LogFilteredData: " << lineNum
                          << " matches size " << matching_lines_.size();
        }
    }
    else if ( visibility_ == MarksOnly ) {
        if ( lineNum.get() < marks_.size() )
            line = marks_.getLineMarkedByIndex( lineNum.get() );
        else
            LOG(logERROR) << "Index too big in LogFilteredData: " << lineNum
                          << " marks size " << marks_.size();
    }
    else {
        // Regenerate the cache if needed
        if ( filteredItemsCacheDirty_ )
            regenerateFilteredItemsCache();

        if ( lineNum.get() < filteredItemsCache_.size() )
            line = filteredItemsCache_[ lineNum.get() ].lineNumber();
        else
            LOG(logERROR) << "Index too big in LogFilteredData: " << lineNum
                          << " fic size " << filteredItemsCache_.size();
    }

    return line;
}

LineNumber LogFilteredData::findFilteredLine( LineNumber lineNum ) const
{
    auto lineIndex = maxValue<LineNumber>();

    if ( visibility_ == MatchesOnly ) {
        lineIndex = lookupLineNumber( matching_lines_.begin(),
                                      matching_lines_.end(),
                                      lineNum );
    }
    else if ( visibility_ == MarksOnly ) {
        lineIndex = lookupLineNumber( marks_.begin(),
                                      marks_.end(),
                                      lineNum );
    }
    else {
        // Regenerate the cache if needed
        if ( filteredItemsCacheDirty_ )
            regenerateFilteredItemsCache();

        lineIndex = lookupLineNumber( filteredItemsCache_.begin(),
                                      filteredItemsCache_.end(),
                                      lineNum );

    }

    return lineIndex;
}

// Implementation of the virtual function.
QString LogFilteredData::doGetLineString( LineNumber lineNum ) const
{
    const auto line = findLogDataLine( lineNum );
    return sourceLogData_->getLineString( line );
}

// Implementation of the virtual function.
QString LogFilteredData::doGetExpandedLineString( LineNumber lineNum ) const
{
    const auto line = findLogDataLine( lineNum );
    return sourceLogData_->getExpandedLineString( line );
}

// Implementation of the virtual function.
std::vector<QString> LogFilteredData::doGetLines( LineNumber first_line, LinesCount number ) const
{
    std::vector<QString> list;
    list.reserve( number.get() );

    for ( auto i = first_line; i < first_line + number; ++i ) {
        list.emplace_back( doGetLineString( i ) );
    }

    return list;
}

// Implementation of the virtual function.
std::vector<QString> LogFilteredData::doGetExpandedLines( LineNumber first_line, LinesCount number ) const
{
    std::vector<QString> list;
    list.reserve( number.get() );

    for ( auto i = first_line; i < first_line + number; ++i ) {
        list.emplace_back( doGetExpandedLineString( i ) );
    }

    return list;
}

// Implementation of the virtual function.
LinesCount LogFilteredData::doGetNbLine() const
{
    size_t nbLines {};

    if ( visibility_ == MatchesOnly )
        nbLines = matching_lines_.size();
    else if ( visibility_ == MarksOnly )
        nbLines = marks_.size();
    else {
        // Regenerate the cache if needed (hopefully most of the time
        // it won't be necessarily)
        if ( filteredItemsCacheDirty_ )
            regenerateFilteredItemsCache();
        nbLines = filteredItemsCache_.size();
    }

    return LinesCount( static_cast<LinesCount::UnderlyingType>( nbLines ) );
}

// Implementation of the virtual function.
LineLength LogFilteredData::doGetMaxLength() const
{
    LineLength max_length;

    if ( visibility_ == MatchesOnly )
        max_length = maxLength_;
    else if ( visibility_ == MarksOnly )
        max_length = maxLengthMarks_;
    else
        max_length = qMax( maxLength_, maxLengthMarks_ );

    return max_length;
}

// Implementation of the virtual function.
LineLength LogFilteredData::doGetLineLength( LineNumber lineNum ) const
{
    LineNumber line = findLogDataLine( lineNum );
    return sourceLogData_->getLineLength( line );
}

void LogFilteredData::doSetDisplayEncoding( const char* encoding )
{
    LOG(logDEBUG) << "AbstractLogData::setDisplayEncoding: " << encoding;
}

QTextCodec* LogFilteredData::doGetDisplayEncoding() const
{
    return sourceLogData_->getDisplayEncoding();
}

// TODO: We might be a bit smarter and not regenerate the whole thing when
// e.g. stuff is added at the end of the search.
void LogFilteredData::regenerateFilteredItemsCache() const
{
    LOG(logDEBUG) << "regenerateFilteredItemsCache";

    filteredItemsCache_.clear();
    filteredItemsCache_.reserve( matching_lines_.size() + marks_.size() );
    // (it's an overestimate but probably not by much so it's fine)

    auto i = matching_lines_.cbegin();
    Marks::const_iterator j = marks_.begin();

    while ( ( i != matching_lines_.cend() ) || ( j != marks_.end() ) ) {

        const auto next_mark =
            ( j != marks_.end() ) ? j->lineNumber() : maxValue<LineNumber>();

        const auto next_match =
            ( i != matching_lines_.cend() ) ? i->lineNumber() : maxValue<LineNumber>();

        LineNumber line;
        FilteredLineType lineType = Match;

        if ( next_mark <= next_match ) {
            // LOG(logDEBUG) << "Add mark at " << next_mark;
            line = next_mark;
            lineType = Mark;
            ++j;
        }

        if ( next_mark >= next_match ) {
            // LOG(logDEBUG) << "Add match at " << next_match;
            line = next_match;

            // We choose a Mark over a Match if a line is both,
            // just an arbitrary choice really.
            lineType = lineType == Mark ? Mark : Match;

            ++i;
        }

        filteredItemsCache_.emplace_back( line, lineType );
    }

    filteredItemsCacheDirty_ = false;

    LOG(logDEBUG) << "finished regenerateFilteredItemsCache, size " << filteredItemsCache_.size();
}
