/*
 * Copyright (C) 2009, 2010 Nicolas Bonnefon and other contributors
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

#include <QFile>
#include <QtConcurrent>

#include "log.h"

#include "logdata.h"
#include "logfiltereddataworkerthread.h"

#include "configuration.h"
#include "persistentinfo.h"

#include <moodycamel/blockingconcurrentqueue.h>

#include <chrono>
#include <cmath>

namespace {
struct PartialSearchResults {
    PartialSearchResults() = default;

    PartialSearchResults( const PartialSearchResults& ) = delete;
    PartialSearchResults( PartialSearchResults&& ) = default;

    PartialSearchResults& operator=( const PartialSearchResults& ) = delete;
    PartialSearchResults& operator=( PartialSearchResults&& ) = default;

    SearchResultArray matchingLines;
    LineLength maxLength;

    LineNumber chunkStart;
    LinesCount processedLines;
};

struct SearchBlockData {
    SearchBlockData() = default;

    SearchBlockData( const SearchBlockData& ) = delete;
    SearchBlockData( SearchBlockData&& ) = default;

    SearchBlockData& operator=( const SearchBlockData& ) = delete;
    SearchBlockData& operator=( SearchBlockData&& ) = default;

    LineNumber chunkStart;
    std::vector<QString> lines;
    PartialSearchResults results;
};

PartialSearchResults filterLines( const QRegularExpression& regex,
                                  const std::vector<QString>& lines, LineNumber chunkStart )
{
    LOG( logDEBUG ) << "Filter lines at " << chunkStart;
    PartialSearchResults results;
    results.chunkStart = chunkStart;
    results.processedLines = LinesCount( static_cast<LinesCount::UnderlyingType>( lines.size() ) );
    for ( size_t i = 0; i < lines.size(); ++i ) {
        const auto& l = lines.at( i );
        if ( regex.match( l ).hasMatch() ) {
            results.maxLength
                = qMax( results.maxLength, AbstractLogData::getUntabifiedLength( l ) );
            results.matchingLines.emplace_back(
                chunkStart + LinesCount( static_cast<LinesCount::UnderlyingType>( i ) ) );
        }
    }
    return results;
}
} // namespace

void SearchData::getAll( LineLength* length, SearchResultArray* matches, LinesCount* lines ) const
{
    QMutexLocker locker( &dataMutex_ );

    *length = maxLength_;
    *lines = nbLinesProcessed_;
    *matches = matches_;
}

void SearchData::setAll( LineLength length, SearchResultArray&& matches )
{
    QMutexLocker locker( &dataMutex_ );

    maxLength_ = length;
    matches_ = matches;
}

void SearchData::addAll( LineLength length, const SearchResultArray& matches, LinesCount lines )
{
    using std::begin;
    using std::end;

    QMutexLocker locker( &dataMutex_ );

    maxLength_ = qMax( maxLength_, length );
    nbLinesProcessed_ = qMax( nbLinesProcessed_, lines );

    if ( !matches.empty() ) {
        auto insertIt = std::lower_bound( begin( matches_ ), end( matches_ ), matches.front() );
        assert( insertIt == end( matches_ ) || ! ( *insertIt < matches.back() ) );
        matches_.insert( insertIt, begin( matches ), end( matches ) );
    }
}

LinesCount SearchData::getNbMatches() const
{
    QMutexLocker locker( &dataMutex_ );

    return LinesCount( static_cast<LinesCount::UnderlyingType>( matches_.size() ) );
}

void SearchData::clear()
{
    QMutexLocker locker( &dataMutex_ );

    maxLength_ = LineLength( 0 );
    nbLinesProcessed_ = LinesCount( 0 );
    matches_.clear();
}

LineNumber SearchData::getLastMatchedLineNumber() const
{
    QMutexLocker locker( &dataMutex_ );

    return matches_.empty() ? LineNumber{ 0 } : matches_.back().lineNumber();
}

LogFilteredDataWorkerThread::LogFilteredDataWorkerThread( const LogData* sourceLogData )
    : QThread()
    , mutex_()
    , operationRequestedCond_()
    , nothingToDoCond_()
    , searchData_()
{
    operationRequested_ = nullptr;
    sourceLogData_ = sourceLogData;
}

LogFilteredDataWorkerThread::~LogFilteredDataWorkerThread()
{
    {
        QMutexLocker locker( &mutex_ );
        terminate_.set();
        operationRequestedCond_.wakeAll();
    }
    wait();
}

void LogFilteredDataWorkerThread::search( const QRegularExpression& regExp, LineNumber startLine,
                                          LineNumber endLine )
{
    QMutexLocker locker( &mutex_ ); // to protect operationRequested_

    LOG( logDEBUG ) << "Search requested";

    // If an operation is ongoing, we will block
    while ( ( operationRequested_ != nullptr ) )
        nothingToDoCond_.wait( &mutex_ );

    interruptRequested_.clear();
    operationRequested_ = new FullSearchOperation( sourceLogData_, regExp, startLine, endLine,
                                                   &interruptRequested_ );
    operationRequestedCond_.wakeAll();
}

void LogFilteredDataWorkerThread::updateSearch( const QRegularExpression& regExp,
                                                LineNumber startLine, LineNumber endLine,
                                                LineNumber position )
{
    QMutexLocker locker( &mutex_ ); // to protect operationRequested_

    LOG( logDEBUG ) << "Search requested";

    // If an operation is ongoing, we will block
    while ( ( operationRequested_ != nullptr ) )
        nothingToDoCond_.wait( &mutex_ );

    interruptRequested_.clear();
    operationRequested_ = new UpdateSearchOperation( sourceLogData_, regExp, startLine, endLine,
                                                     &interruptRequested_, position );
    operationRequestedCond_.wakeAll();
}

void LogFilteredDataWorkerThread::interrupt()
{
    LOG( logDEBUG ) << "Search interruption requested";

    interruptRequested_.set();

    // We wait for the interruption to be done
    {
        QMutexLocker locker( &mutex_ );
        while ( ( operationRequested_ != nullptr ) )
            nothingToDoCond_.wait( &mutex_ );
    }
}

// This will do an atomic copy of the object
void LogFilteredDataWorkerThread::getSearchResult( LineLength* maxLength,
                                                   SearchResultArray* searchMatches,
                                                   LinesCount* nbLinesProcessed )
{
    searchData_.getAll( maxLength, searchMatches, nbLinesProcessed );
}

// This is the thread's main loop
void LogFilteredDataWorkerThread::run()
{
    QMutexLocker locker( &mutex_ );

    forever
    {
        while ( !terminate_ && ( operationRequested_ == nullptr ) )
            operationRequestedCond_.wait( &mutex_ );
        LOG( logDEBUG ) << "Worker thread signaled";

        // Look at what needs to be done
        if ( terminate_ )
            return; // We must die

        if ( operationRequested_ ) {
            connect( operationRequested_, &SearchOperation::searchProgressed, this,
                     &LogFilteredDataWorkerThread::searchProgressed, Qt::QueuedConnection );

            // Run the search operation
            operationRequested_->start( searchData_ );

            LOG( logDEBUG ) << "... finished copy in workerThread.";

            emit searchFinished();
            delete operationRequested_;
            operationRequested_ = nullptr;
            nothingToDoCond_.wakeAll();
        }
    }
}

//
// Operations implementation
//

SearchOperation::SearchOperation( const LogData* sourceLogData, const QRegularExpression& regExp,
                                  LineNumber startLine, LineNumber endLine,
                                  AtomicFlag* interruptRequest )
    : regexp_( regExp )
    , sourceLogData_( sourceLogData )
    , startLine_( startLine )
    , endLine_( endLine )

{
    interruptRequested_ = interruptRequest;
}

void SearchOperation::doSearch( SearchData& searchData, LineNumber initialLine )
{
    const auto& config = Persistent<Configuration>( "settings" );

    const auto nbSourceLines = sourceLogData_->getNbLine();
    LineLength maxLength = 0_length;
    LinesCount nbMatches = searchData.getNbMatches();

    const auto nbLinesInChunk = LinesCount( config.searchReadBufferSizeLines() );

    LOG( logDEBUG ) << "Searching from line " << initialLine << " to " << nbSourceLines;

    if ( initialLine < startLine_ ) {
        initialLine = startLine_;
    }

    const auto endLine = qMin( LineNumber( nbSourceLines.get() ), endLine_ );

    using namespace std::chrono;
    high_resolution_clock::time_point t1 = high_resolution_clock::now();

    QSemaphore searchCompleted;
    QSemaphore blocksDone;

    using SearchBlockQueue = moodycamel::BlockingConcurrentQueue<SearchBlockData>;
    using ProcessMatchQueue = moodycamel::BlockingConcurrentQueue<PartialSearchResults>;

    SearchBlockQueue searchBlockQueue;
    ProcessMatchQueue processMatchQueue;

    std::vector<QFuture<void>> matchers;

    const auto matchingThreadsCount = [&config]()
    {
        if ( !config.useParallelSearch() ) {
            return 1;
        }

        return qMax( 1,  config.searchThreadPoolSize() == 0 
                        ? QThread::idealThreadCount() - 1 
                        : static_cast<int>( config.searchThreadPoolSize() ) );    
    }();

    LOG( logINFO ) << "Using " << matchingThreadsCount << " matching threads";

    auto localThreadPool = std::make_unique<QThreadPool>();
    localThreadPool->setMaxThreadCount( matchingThreadsCount + 2 );

    const auto makeMatcher = [this, &searchBlockQueue, &processMatchQueue,
                              pool = localThreadPool.get()]( size_t index ) {
        // copy and optimize regex for each thread
        auto regexp = QRegularExpression{ regexp_.pattern(), regexp_.patternOptions() };
        regexp.optimize();
        return QtConcurrent::run( pool, [regexp, index, &searchBlockQueue, &processMatchQueue]() {
            auto cToken = moodycamel::ConsumerToken{ searchBlockQueue };
            auto pToken = moodycamel::ProducerToken{ processMatchQueue };

            for ( ;; ) {
                SearchBlockData blockData;
                searchBlockQueue.wait_dequeue( cToken, blockData );

                LOG( logDEBUG ) << "Searcher " << index << " " << blockData.chunkStart;

                const auto lastBlock = blockData.lines.empty();

                if ( !lastBlock ) {
                    blockData.results
                        = filterLines( regexp, blockData.lines, blockData.chunkStart );
                }

                LOG( logDEBUG ) << "Searcher " << index << " sending matches "
                                << blockData.results.matchingLines.size();

                processMatchQueue.enqueue( pToken, std::move( blockData.results ) );

                if ( lastBlock ) {
                    LOG( logDEBUG ) << "Searcher " << index << " last block";
                    return;
                }
            }
        } );
    };

    for ( int i = 0; i < matchingThreadsCount; ++i ) {
        matchers.emplace_back( makeMatcher( i ) );
    }

    auto processMatches = QtConcurrent::run( localThreadPool.get(), [&]() {
        auto cToken = moodycamel::ConsumerToken{ processMatchQueue };

        size_t matchersDone = 0;

        int reportedPercentage = 0;
        auto reportedMatches = nbMatches;
        LinesCount totalProcessedLines = 0_lcount;

        const auto totalLines = endLine - initialLine;

        for ( ;; ) {
            PartialSearchResults matchResults;
            processMatchQueue.wait_dequeue( cToken, matchResults );

            LOG( logDEBUG ) << "Combining match results from " << matchResults.chunkStart;

            if ( matchResults.processedLines.get() ) {

                maxLength = qMax( maxLength, matchResults.maxLength );
                nbMatches += LinesCount(
                    static_cast<LinesCount::UnderlyingType>( matchResults.matchingLines.size() ) );

                const auto processedLines = LinesCount{ matchResults.chunkStart.get()
                                                        + matchResults.processedLines.get() };

                totalProcessedLines += matchResults.processedLines;

                // After each block, copy the data to shared data
                // and update the client
                searchData.addAll( maxLength, matchResults.matchingLines, processedLines );

                LOG( logDEBUG ) << "done Searching chunk starting at " << matchResults.chunkStart
                                << ", " << matchResults.processedLines << " lines read.";

                blocksDone.release( matchResults.processedLines.get() );
            }
            else {
                matchersDone++;
            }

            const int percentage 
                = static_cast<int>( std::floor( 100.f * ( totalProcessedLines ).get() / totalLines.get() ) );

            if ( percentage > reportedPercentage || nbMatches > reportedMatches ) {

                emit searchProgressed( nbMatches, std::min( 99, percentage ), initialLine );

                reportedPercentage = percentage;
                reportedMatches = nbMatches;
            }

            if ( matchersDone == matchers.size() ) {
                searchCompleted.release();
                return;
            }
        }
    } );

    auto pToken =  moodycamel::ProducerToken{ searchBlockQueue };

    blocksDone.release( nbLinesInChunk.get() * ( static_cast<uint32_t>( matchers.size() ) + 1 ) );

    for ( auto chunkStart = initialLine; chunkStart < endLine;
          chunkStart = chunkStart + nbLinesInChunk ) {
        if ( *interruptRequested_ )
            break;

        LOG( logDEBUG ) << "Reading chunk starting at " << chunkStart;

        const auto linesInChunk
            = LinesCount( qMin( nbLinesInChunk.get(), ( endLine - chunkStart ).get() ) );
        auto lines = sourceLogData_->getLines( chunkStart, linesInChunk );

        LOG( logDEBUG ) << "Sending chunk starting at " << chunkStart << ", " << lines.size()
                        << " lines read.";

        blocksDone.acquire( static_cast<uint32_t>( lines.size() ) );

        searchBlockQueue.enqueue( pToken,
                                  { chunkStart, std::move( lines ), PartialSearchResults{} } );

        LOG( logDEBUG ) << "Sent chunk starting at " << chunkStart << ", " << lines.size()
                        << " lines read.";
    }

    for ( size_t i = 0; i < matchers.size(); ++i ) {
        searchBlockQueue.enqueue( pToken,
                                  { endLine, std::vector<QString>{}, PartialSearchResults{} } );
    }

    searchCompleted.acquire();

    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>( t2 - t1 ).count();

    LOG( logINFO ) << "Searching done, took " << duration / 1000.f << " ms";
    LOG( logINFO ) << "Searching perf "
                   << static_cast<uint32_t>(
                          std::floor( 1000 * 1000.f * ( endLine - initialLine ).get() / duration ) )
                   << " lines/s";

    emit searchProgressed( nbMatches, 100, initialLine );
}

// Called in the worker thread's context
void FullSearchOperation::start( SearchData& searchData )
{
    // Clear the shared data
    searchData.clear();

    doSearch( searchData, 0_lnum );
}

// Called in the worker thread's context
void UpdateSearchOperation::start( SearchData& searchData )
{
    auto initial_line = initialPosition_;

    if ( initial_line.get() >= 1 && searchData.getLastMatchedLineNumber() != initial_line ) {
        // We need to re-search the last line because it might have
        // been updated (if it was not LF-terminated)
        --initial_line;
    }

    doSearch( searchData, initial_line );
}
