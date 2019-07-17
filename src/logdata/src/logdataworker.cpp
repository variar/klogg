/*
 * Copyright (C) 2009, 2010, 2014, 2015 Nicolas Bonnefon and other contributors
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
#include <QTextStream>
#include <QtConcurrent>

#include <moodycamel/readerwriterqueue.h>

#include "log.h"

#include "logdata.h"
#include "logdataworker.h"

#include "configuration.h"

#include <chrono>
#include <cmath>

qint64 IndexingData::getSize() const
{
    QMutexLocker locker( &dataMutex_ );

    return indexedSize_;
}

LineLength IndexingData::getMaxLength() const
{
    QMutexLocker locker( &dataMutex_ );

    return maxLength_;
}

LinesCount IndexingData::getNbLines() const
{
    QMutexLocker locker( &dataMutex_ );

    return LinesCount( linePosition_.size() );
}

LineOffset IndexingData::getPosForLine( LineNumber line ) const
{
    QMutexLocker locker( &dataMutex_ );

    return linePosition_.at( line.get() );
}

QTextCodec* IndexingData::getEncodingGuess() const
{
    QMutexLocker locker( &dataMutex_ );
    return encodingGuess_;
}

void IndexingData::forceEncoding( QTextCodec* codec )
{
    QMutexLocker locker( &dataMutex_ );
    encodingForced_ = codec;
}

QTextCodec* IndexingData::getForcedEncoding() const
{
    QMutexLocker locker( &dataMutex_ );
    return encodingForced_;
}

void IndexingData::addAll( qint64 size, LineLength length,
                           const FastLinePositionArray& linePosition, QTextCodec* encoding )

{
    QMutexLocker locker( &dataMutex_ );

    indexedSize_ += size;
    maxLength_ = qMax( maxLength_, length );
    linePosition_.append_list( linePosition );

    encodingGuess_ = encoding;
}

void IndexingData::clear()
{
    QMutexLocker locker( &dataMutex_ );

    maxLength_ = 0_length;
    indexedSize_ = 0;
    linePosition_ = LinePositionArray();
    encodingGuess_ = QTextCodec::codecForLocale();
    encodingForced_ = nullptr;
}

LogDataWorker::LogDataWorker( IndexingData& indexing_data )
    : indexing_data_( indexing_data )
{
    connect( &operationWatcher_, &QFutureWatcher<bool>::finished, this,
             &LogDataWorker::onOperationFinished, Qt::QueuedConnection );
}

LogDataWorker::~LogDataWorker()
{
    interruptRequest_.set();
    QMutexLocker locker( &mutex_ );
    operationWatcher_.waitForFinished();
}

void LogDataWorker::attachFile( const QString& fileName )
{
    QMutexLocker locker( &mutex_ ); // to protect fileName_
    fileName_ = fileName;
}

void LogDataWorker::indexAll( QTextCodec* forcedEncoding )
{
    QMutexLocker locker( &mutex_ );
    LOG( logDEBUG ) << "FullIndex requested";

    operationWatcher_.waitForFinished();
    interruptRequest_.clear();

    operationFuture_ = QtConcurrent::run( [this, forcedEncoding, fileName = fileName_] {
        auto operationRequested = std::make_unique<FullIndexOperation>(
            fileName, indexing_data_, interruptRequest_, forcedEncoding );
        return connectSignalsAndRun( operationRequested.get() );
    } );

    operationWatcher_.setFuture( operationFuture_ );
}

void LogDataWorker::indexAdditionalLines()
{
    QMutexLocker locker( &mutex_ );
    LOG( logDEBUG ) << "AddLines requested";

    operationWatcher_.waitForFinished();
    interruptRequest_.clear();

    operationFuture_ = QtConcurrent::run( [this, fileName = fileName_] {
        auto operationRequested = std::make_unique<PartialIndexOperation>( fileName, indexing_data_,
                                                                           interruptRequest_ );
        return connectSignalsAndRun( operationRequested.get() );
    } );

    operationWatcher_.setFuture( operationFuture_ );
}

bool LogDataWorker::connectSignalsAndRun( IndexOperation* operationRequested )
{
    connect( operationRequested, &IndexOperation::indexingProgressed, this,
             &LogDataWorker::indexingProgressed );

    return operationRequested->start();
}

void LogDataWorker::interrupt()
{
    LOG( logINFO ) << "Load interrupt requested";
    interruptRequest_.set();
}

void LogDataWorker::onOperationFinished()
{
    if ( operationWatcher_.result() ) {
        LOG( logDEBUG ) << "... finished copy in workerThread.";
        emit indexingFinished( LoadingStatus::Successful );
    }
    else {
        LOG( logINFO ) << "indexing interrupted";
        emit indexingFinished( LoadingStatus::Interrupted );
    }
}

//
// Operations implementation
//

FastLinePositionArray IndexOperation::parseDataBlock( LineOffset::UnderlyingType block_beginning,
                                                      const QByteArray& block,
                                                      IndexingState& state ) const
{
    state.max_length = 0;
    FastLinePositionArray line_positions;

    int pos_within_block = 0;

    const auto adjustToCharWidth
        = [&state]( int pos ) { return pos - state.encodingParams.getBeforeCrOffset(); };

    const auto expandTabs = [&]( const char* search_start, int line_size ) {
        auto tab_search_start = search_start;
        auto next_tab
            = reinterpret_cast<const char*>( std::memchr( tab_search_start, '\t', line_size ) );
        while ( next_tab != nullptr ) {
            pos_within_block = adjustToCharWidth( next_tab - block.data() );

            LOG( logDEBUG1 ) << "Tab at " << pos_within_block;

            state.additional_spaces
                += AbstractLogData::tabStop
                   - ( static_cast<int>( ( block_beginning - state.pos ) + pos_within_block
                                         + state.additional_spaces )
                       % AbstractLogData::tabStop )
                   - 1;

            const auto tab_substring_size = next_tab - tab_search_start;
            line_size -= tab_substring_size;
            tab_search_start = next_tab + 1;
            if ( line_size > 0 ) {
                next_tab = reinterpret_cast<const char*>(
                    std::memchr( tab_search_start, '\t', line_size ) );
            }
            else {
                next_tab = nullptr;
            }
        }
    };

    while ( pos_within_block != -1 ) {
        pos_within_block = qMax( static_cast<int>( state.pos - block_beginning ), 0 );

        // Looking for the next \n, expanding tabs in the process

        const auto search_start = block.data() + pos_within_block;
        const auto search_line_size = block.size() - pos_within_block;

        if ( search_line_size > 0 ) {
            const auto next_line_feed = reinterpret_cast<const char*>(
                std::memchr( search_start, '\n', search_line_size ) );

            if ( next_line_feed != nullptr ) {
                expandTabs( search_start, next_line_feed - search_start );
                pos_within_block = adjustToCharWidth( next_line_feed - block.data() );
                LOG( logDEBUG1 ) << "LF at " << pos_within_block;
            }
            else {
                expandTabs( search_start, search_line_size );
                pos_within_block = -1;
            }
        }
        else {
            pos_within_block = -1;
        }

        // When a end of line has been found...
        if ( pos_within_block != -1 ) {
            state.end = pos_within_block + block_beginning;
            const auto length = static_cast<LineLength::UnderlyingType>(
                state.end - state.pos + state.additional_spaces );
            if ( length > state.max_length )
                state.max_length = length;

            state.pos = state.end + state.encodingParams.lineFeedWidth;
            state.additional_spaces = 0;
            line_positions.append( LineOffset( state.pos ) );
        }
    }

    return line_positions;
}

void IndexOperation::guessEncoding( const QByteArray& block, IndexingState& state ) const
{
    if ( !state.fileTextCodec ) {
        state.fileTextCodec = indexing_data_.getForcedEncoding();
        if ( !state.fileTextCodec ) {
            state.fileTextCodec = EncodingDetector::getInstance().detectEncoding( block );
        }

        state.encodingParams = EncodingParameters( state.fileTextCodec );
        LOG( logINFO ) << "Encoding " << state.fileTextCodec->name().toStdString()
                       << ", Char width " << state.encodingParams.lineFeedWidth;
    }

    if ( !state.encodingGuess ) {
        state.encodingGuess = EncodingDetector::getInstance().detectEncoding( block );
        LOG( logINFO ) << "Encoding guess " << state.encodingGuess->name().toStdString();
    }
}

auto IndexOperation::setupIndexingProcess( IndexingState& indexingState )
{
    using BlockData = std::pair<LineOffset::UnderlyingType, QByteArray>;
    using BlockQueue = moodycamel::BlockingReaderWriterQueue<BlockData>;
    auto blockQueue = std::make_unique<BlockQueue>();
    auto threadPool = std::make_unique<QThreadPool>();
    threadPool->setMaxThreadCount( 1 );

    auto consumerFuture
        = QtConcurrent::run( threadPool.get(), [queue = blockQueue.get(), &indexingState, this] {
              for ( ;; ) {
                  BlockData blockData;
                  queue->wait_dequeue( blockData );

                  const auto block_beginning = blockData.first;
                  const auto& block = blockData.second;

                  LOG( logDEBUG ) << "Indexing block " << block_beginning;

                  if ( block.isEmpty() ) {
                      indexingState.indexingSem.release();
                      return;
                  }

                  guessEncoding( block, indexingState );

                  auto line_positions = parseDataBlock( block_beginning, block, indexingState );
                  indexing_data_.addAll( block.length(), LineLength( indexingState.max_length ),
                                         line_positions, indexingState.encodingGuess );

                  // Update the caller for progress indication
                  const auto progress = static_cast<int>(
                      std::floor( ( indexingState.file_size > 0 )
                                      ? indexingState.pos * 100.f / indexingState.file_size
                                      : 100 ) );
                  emit indexingProgressed( progress );

                  indexingState.blockSem.release( block.size() );
              }
          } );

    std::function<void( BlockData )> blockSender
        = [queue = blockQueue.get()]( BlockData block ) { queue->enqueue( std::move( block ) ); };

    return std::make_tuple( std::move( blockSender ), std::move( blockQueue ),
                            std::move( consumerFuture ), std::move( threadPool ) );
}

void IndexOperation::doIndex( LineOffset initialPosition )
{
    const auto& config = Persistable::get<Configuration>();

    const uint32_t sizeChunk = 1024 * 1024;
    const auto prefetchBufferSize = config.indexReadBufferSizeMb() * sizeChunk;

    QFile file( fileName_ );

    if ( file.isOpen() || file.open( QIODevice::ReadOnly ) ) {

        IndexingState state;
        state.pos = initialPosition.get();
        state.end = 0ll;
        state.additional_spaces = 0;
        state.max_length = 0;
        state.encodingGuess = nullptr;
        state.fileTextCodec = nullptr;
        state.file_size = file.size();

        auto process = setupIndexingProcess( state );
        auto blockSender = std::get<0>( process );

        state.blockSem.release( prefetchBufferSize );

        using namespace std::chrono;
        using clock = high_resolution_clock;
        clock::time_point t1 = clock::now();

        size_t ioDuration{};

        file.seek( state.pos );
        while ( !file.atEnd() ) {

            if ( interruptRequest_ )
                break;

            const auto block_beginning = file.pos();

            clock::time_point ioT1 = clock::now();
            const auto block = file.read( sizeChunk );
            clock::time_point ioT2 = clock::now();

            ioDuration += duration_cast<milliseconds>( ioT2 - ioT1 ).count();

            state.blockSem.acquire( block.size() );

            LOG( logDEBUG ) << "Sending block " << block_beginning;
            blockSender( std::make_pair( block_beginning, block ) );
        }

        blockSender( std::make_pair( state.file_size, QByteArray{} ) );

        {
            state.indexingSem.acquire();
        }

        // Check if there is a non LF terminated line at the end of the file
        if ( !interruptRequest_ && state.file_size > state.pos ) {
            LOG( logWARNING ) << "Non LF terminated file, adding a fake end of line";

            FastLinePositionArray line_position;
            line_position.append( LineOffset( state.file_size + 1 ) );
            line_position.setFakeFinalLF();

            indexing_data_.addAll( 0, 0_length, line_position, state.encodingGuess );
        }

        high_resolution_clock::time_point t2 = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>( t2 - t1 ).count();

        LOG( logINFO ) << "Indexing done, took " << duration << " ms, io " << ioDuration << " ms";
        LOG( logINFO ) << "Indexing perf "
                       << ( 1000.f * state.file_size / duration ) / ( 1024 * 1024 ) << " MiB/s";
    }
    else {
        // TODO: Check that the file is seekable?
        // If the file cannot be open, we do as if it was empty
        LOG( logWARNING ) << "Cannot open file " << fileName_.toStdString();

        indexing_data_.clear();

        emit indexingProgressed( 100 );
    }
}

// Called in the worker thread's context
bool FullIndexOperation::start()
{
    LOG( logDEBUG ) << "FullIndexOperation::start(), file " << fileName_.toStdString();

    LOG( logDEBUG ) << "FullIndexOperation: Starting the count...";

    emit indexingProgressed( 0 );

    // First empty the index
    indexing_data_.clear();
    indexing_data_.forceEncoding( forcedEncoding_ );

    doIndex( 0_offset );

    LOG( logDEBUG ) << "FullIndexOperation: ... finished counting."
                       "interrupt = "
                    << static_cast<bool>( interruptRequest_ );

    return ( interruptRequest_ ? false : true );
}

bool PartialIndexOperation::start()
{
    LOG( logDEBUG ) << "PartialIndexOperation::start(), file " << fileName_.toStdString();

    const auto initial_position = LineOffset( indexing_data_.getSize() );

    LOG( logDEBUG ) << "PartialIndexOperation: Starting the count at " << initial_position
                    << " ...";

    emit indexingProgressed( 0 );

    doIndex( initial_position );

    LOG( logDEBUG ) << "PartialIndexOperation: ... finished counting.";

    return ( interruptRequest_ ? false : true );
}
