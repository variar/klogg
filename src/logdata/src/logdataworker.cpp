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

#include <chrono>
#include <cmath>
#include <exception>
#include <functional>
#include <oneapi/tbb/flow_graph.h>
#include <string_view>
#include <thread>

#include <QFile>
#include <QFileInfo>
#include <QMessageBox>

#include "configuration.h"
#include "dispatch_to.h"
#include "encodingdetector.h"
#include "issuereporter.h"
#include "linetypes.h"
#include "log.h"
#include "logdata.h"
#include "progress.h"
#include "readablesize.h"

#include "logdataworker.h"

constexpr int IndexingBlockSize = 1 * 1024 * 1024;

qint64 IndexingData::getIndexedSize() const
{
    return hash_.size;
}

IndexedHash IndexingData::getHash() const
{
    return hash_;
}

LineLength IndexingData::getMaxLength() const
{
    return maxLength_;
}

LinesCount IndexingData::getNbLines() const
{
    return LinesCount( linePosition_.size() );
}

LineOffset IndexingData::getPosForLine( LineNumber line ) const
{
    return linePosition_.at( line.get() );
}

QTextCodec* IndexingData::getEncodingGuess() const
{
    return encodingGuess_;
}

void IndexingData::setEncodingGuess( QTextCodec* codec )
{
    encodingGuess_ = codec;
}

void IndexingData::forceEncoding( QTextCodec* codec )
{
    encodingForced_ = codec;
}

QTextCodec* IndexingData::getForcedEncoding() const
{
    return encodingForced_;
}

void IndexingData::addAll( const QByteArray& block, LineLength length,
                           const FastLinePositionArray& linePosition, QTextCodec* encoding )

{
    maxLength_ = qMax( maxLength_, length );
    linePosition_.append_list( linePosition );

    if ( !block.isEmpty() ) {
        hash_.size += block.size();

        if ( !useFastModificationDetection_ ) {
            hashBuilder_.addData( block.data(), static_cast<size_t>( block.size() ) );
            hash_.fullDigest = hashBuilder_.digest();
        }
    }

    encodingGuess_ = encoding;
}

int IndexingData::getProgress() const
{
    return progress_;
}

void IndexingData::setProgress( int progress )
{
    progress_ = progress;
}

void IndexingData::clear()
{
    maxLength_ = 0_length;
    hash_ = {};
    hashBuilder_.reset();
    linePosition_ = LinePositionArray();
    encodingGuess_ = nullptr;
    encodingForced_ = nullptr;

    progress_ = {};

    const auto& config = Configuration::get();
    useFastModificationDetection_ = config.fastModificationDetection();
}

size_t IndexingData::allocatedSize() const
{
    return linePosition_.allocatedSize();
}

LogDataWorker::LogDataWorker( const std::shared_ptr<IndexingData>& indexing_data )
    : indexing_data_( indexing_data )
{
}

void LogDataWorker::waitForDone()
{
    operationsExecuter_.wait();
    interruptRequest_.clear();
}

LogDataWorker::~LogDataWorker() noexcept
{
    try {
        interruptRequest_.set();
        ScopedLock locker( mutex_ );
        waitForDone();
    } catch ( const std::exception& e ) {
        LOG_ERROR << "Failed to destroy LogDataWorker: " << e.what();
    }
}

void LogDataWorker::attachFile( const QString& fileName )
{
    ScopedLock locker( mutex_ ); // to protect fileName_
    fileName_ = fileName;
}

void LogDataWorker::indexAll( QTextCodec* forcedEncoding )
{
    ScopedLock locker( mutex_ );
    LOG_DEBUG << "FullIndex requested";

    waitForDone();

    operationsExecuter_.run( [ this, forcedEncoding, fileName = fileName_ ] {
        auto operationRequested = std::make_unique<FullIndexOperation>(
            fileName, indexing_data_, interruptRequest_, forcedEncoding );
        return connectSignalsAndRun( operationRequested.get() );
    } );
}

void LogDataWorker::indexAdditionalLines()
{
    ScopedLock locker( mutex_ );
    LOG_DEBUG << "AddLines requested";

    waitForDone();

    operationsExecuter_.run( [ this, fileName = fileName_ ] {
        auto operationRequested = std::make_unique<PartialIndexOperation>( fileName, indexing_data_,
                                                                           interruptRequest_ );
        return connectSignalsAndRun( operationRequested.get() );
    } );
}

void LogDataWorker::checkFileChanges()
{
    ScopedLock locker( mutex_ );
    LOG_DEBUG << "Check file changes requested";

    waitForDone();

    operationsExecuter_.run( [ this, fileName = fileName_ ] {
        auto operationRequested = std::make_unique<CheckFileChangesOperation>(
            fileName, indexing_data_, interruptRequest_ );

        return connectSignalsAndRun( operationRequested.get() );
    } );
}

OperationResult LogDataWorker::connectSignalsAndRun( IndexOperation* operationRequested )
{
    connect( operationRequested, &IndexOperation::indexingProgressed, this,
             &LogDataWorker::indexingProgressed );

    connect( operationRequested, &IndexOperation::indexingFinished, this,
             &LogDataWorker::onIndexingFinished );

    connect( operationRequested, &IndexOperation::fileCheckFinished, this,
             &LogDataWorker::onCheckFileFinished );

    auto result = operationRequested->run();

    operationRequested->disconnect( this );

    return result;
}

void LogDataWorker::interrupt()
{
    LOG_INFO << "Load interrupt requested";
    interruptRequest_.set();
}

void LogDataWorker::onIndexingFinished( bool result )
{
    if ( result ) {
        LOG_INFO << "finished indexing in worker thread";
        emit indexingFinished( LoadingStatus::Successful );
    }
    else {
        LOG_INFO << "indexing interrupted in worker thread";
        emit indexingFinished( LoadingStatus::Interrupted );
    }
}

void LogDataWorker::onCheckFileFinished( const MonitoredFileStatus result )
{
    LOG_INFO << "checking file finished in worker thread";
    emit checkFileChangesFinished( result );
}

//
// Operations implementation
//
namespace parse_data_block {

std::string_view::size_type findNextMultiByteDelimeter( EncodingParameters encodingParams,
                                                        std::string_view data, char delimeter )
{
    auto nextDelimeter = data.find( delimeter );

    if ( nextDelimeter == std::string_view::npos ) {
        return nextDelimeter;
    }

    const auto isNotDelimeter = [ &encodingParams, data ]( std::string_view::size_type checkPos ) {
        const auto lineFeedWidth
            = static_cast<std::string_view::size_type>( encodingParams.lineFeedWidth );

        const auto isCheckForward = encodingParams.lineFeedIndex == 0;

        if ( isCheckForward && checkPos + lineFeedWidth > data.size() ) {
            return true;
        }
        else if ( !isCheckForward && checkPos < lineFeedWidth - 1 ) {
            return true;
        }

        for ( auto i = 1u; i < lineFeedWidth; ++i ) {
            const auto nextByte = isCheckForward ? data[ checkPos + i ] : data[ checkPos - i ];
            if ( nextByte != '\0' ) {
                return true;
            }
        }

        return false;
    };

    while ( nextDelimeter != std::string_view::npos && isNotDelimeter( nextDelimeter ) ) {
        nextDelimeter = data.find( delimeter, nextDelimeter + 1 );
    }

    return nextDelimeter;
}

std::string_view::size_type findNextSingleByteDelimeter( EncodingParameters, std::string_view data,
                                                         char delimeter )
{
    return data.find( delimeter );
}

qsizetype charOffsetWithinBlock( const char* blockStart, const char* pointer,
                           const EncodingParameters& encodingParams )
{
    return static_cast<qsizetype>( std::distance( blockStart, pointer ) )
           - encodingParams.getBeforeCrOffset();
}

using FindDelimeter = std::string_view::size_type ( * )( EncodingParameters encodingParams,
                                                         std::string_view, char );

LineLength::UnderlyingType
expandTabsInLine( const QByteArray& block, std::string_view blockToExpand, qsizetype posWithinBlock,
                  EncodingParameters encodingParams, FindDelimeter findNextDelimeter,
                  LineLength::UnderlyingType initialAdditionalSpaces = 0 )
{
    auto additionalSpaces = initialAdditionalSpaces;
    while ( !blockToExpand.empty() ) {
        const auto nextTab = findNextDelimeter( encodingParams, blockToExpand, '\t' );
        if ( nextTab == std::string_view::npos ) {
            break;
        }

        const auto tabPosWithinBlock
            = charOffsetWithinBlock( block.data(), blockToExpand.data() + nextTab, encodingParams );

        LOG_DEBUG << "Tab at " << tabPosWithinBlock;

        const auto currentExpandedSize = tabPosWithinBlock - posWithinBlock + additionalSpaces;

        additionalSpaces += TabStop - ( currentExpandedSize % TabStop ) - 1;
        if ( nextTab >= blockToExpand.size() ) {
            break;
        }

        blockToExpand.remove_prefix( nextTab + 1 );
    }

    return additionalSpaces;
}

std::tuple<bool, int, LineLength::UnderlyingType>
findNextLineFeed( const QByteArray& block, qsizetype posWithinBlock, const IndexingState& state,
                  FindDelimeter findNextDelimeter )
{
    const auto searchStart = block.data() + posWithinBlock;
    const auto searchLineSize = static_cast<size_t>( block.size() - posWithinBlock );

    const auto blockView = std::string_view( searchStart, searchLineSize );
    const auto nextLineFeed = findNextDelimeter( state.encodingParams, blockView, '\n' );

    const auto isEndOfBlock = nextLineFeed == std::string_view::npos;

    const auto nextLineSize = !isEndOfBlock ? nextLineFeed : searchLineSize;
    const auto additionalSpaces
        = expandTabsInLine( block, blockView.substr( 0, nextLineSize ), posWithinBlock,
                            state.encodingParams, findNextDelimeter, state.additional_spaces );

    posWithinBlock
        = charOffsetWithinBlock( block.data(), searchStart + nextLineSize, state.encodingParams );

    return std::make_tuple( isEndOfBlock, posWithinBlock, additionalSpaces );
}
} // namespace parse_data_block

FastLinePositionArray IndexOperation::parseDataBlock( LineOffset::UnderlyingType blockBeginning,
                                                      const QByteArray& block,
                                                      IndexingState& state ) const
{
    using namespace parse_data_block;

    FindDelimeter findNextDelimeter;
    if ( state.encodingParams.lineFeedWidth == 1 ) {
        findNextDelimeter = findNextSingleByteDelimeter;
    }
    else {
        findNextDelimeter = findNextMultiByteDelimeter;
    }

    bool isEndOfBlock = false;
    FastLinePositionArray linePositions;

    while ( !isEndOfBlock ) {
        if ( state.pos > blockBeginning + block.size() ) {
            LOG_ERROR << "Trying to parse out of block: " << state.pos << " " << blockBeginning
                      << " " << block.size();
            break;
        }

        auto posWithinBlock
            = static_cast<int>( state.pos >= blockBeginning ? ( state.pos - blockBeginning ) : 0u );

        isEndOfBlock = posWithinBlock == block.size();

        if ( !isEndOfBlock ) {
            std::tie( isEndOfBlock, posWithinBlock, state.additional_spaces )
                = findNextLineFeed( block, posWithinBlock, state, findNextDelimeter );
        }

        const auto currentDataEnd = posWithinBlock + blockBeginning;

        const auto length = ( currentDataEnd - state.pos ) / state.encodingParams.lineFeedWidth
                            + state.additional_spaces;

        state.max_length = std::max( state.max_length, length );

        if ( !isEndOfBlock ) {
            state.end = currentDataEnd;
            state.pos = state.end + state.encodingParams.lineFeedWidth;
            state.additional_spaces = 0;
            linePositions.append( LineOffset( state.pos ) );
        }
    }

    return linePositions;
}

void IndexOperation::guessEncoding( const QByteArray& block,
                                    IndexingData::MutateAccessor& scopedAccessor,
                                    IndexingState& state ) const
{
    if ( !state.encodingGuess ) {
        state.encodingGuess = EncodingDetector::getInstance().detectEncoding( block );
        LOG_INFO << "Encoding guess " << state.encodingGuess->name().toStdString();
    }

    if ( !state.fileTextCodec ) {
        state.fileTextCodec = scopedAccessor.getForcedEncoding();

        if ( !state.fileTextCodec ) {
            state.fileTextCodec = scopedAccessor.getEncodingGuess();
        }

        if ( !state.fileTextCodec ) {
            state.fileTextCodec = state.encodingGuess;
        }
    }

    state.encodingParams = EncodingParameters( state.fileTextCodec );

    LOG_DEBUG << "Encoding " << state.fileTextCodec->name().toStdString() << ", Char width "
              << state.encodingParams.lineFeedWidth;
}

std::chrono::microseconds IndexOperation::readFileInBlocks( QFile& file,
                                                            BlockReader::gateway_type& gw )
{
    using namespace std::chrono;
    using clock = high_resolution_clock;

    LOG_INFO << "Starting IO thread";

    microseconds ioDuration{};
    while ( !file.atEnd() ) {

        if ( interruptRequest_ ) {
            break;
        }

        BlockData blockData{ file.pos(), QByteArray{ IndexingBlockSize, Qt::Uninitialized } };

        clock::time_point ioT1 = clock::now();
        const auto readBytes
            = static_cast<int>( file.read( blockData.second.data(), blockData.second.size() ) );

        if ( readBytes < 0 ) {
            LOG_ERROR << "Reading past the end of file";
            break;
        }

        if ( readBytes < blockData.second.size() ) {
            blockData.second.resize( readBytes );
        }

        clock::time_point ioT2 = clock::now();

        ioDuration += duration_cast<microseconds>( ioT2 - ioT1 );

        LOG_DEBUG << "Sending block " << blockData.first << " size " << blockData.second.size();

        while ( !gw.try_put( blockData ) ) {
            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
        }
    }

    auto lastBlock = std::make_pair( -1, QByteArray{} );
    while ( !gw.try_put( lastBlock ) ) {
        std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
    }

    LOG_INFO << "IO thread done";
    return ioDuration;
}

void IndexOperation::indexNextBlock( IndexingState& state, const BlockData& blockData )
{
    const auto& blockBeginning = blockData.first;
    const auto& block = blockData.second;

    LOG_DEBUG << "Indexing block " << blockBeginning << " start";

    if ( blockBeginning < 0 ) {
        return;
    }

    IndexingData::MutateAccessor scopedAccessor{ indexing_data_.get() };

    guessEncoding( block, scopedAccessor, state );

    if ( !block.isEmpty() ) {
        const auto linePositions = parseDataBlock( blockBeginning, block, state );
        auto maxLength = state.max_length;
        if ( maxLength > std::numeric_limits<LineLength::UnderlyingType>::max() ) {
            LOG_ERROR << "Too long lines " << maxLength;
            maxLength = std::numeric_limits<LineLength::UnderlyingType>::max();
        }

        scopedAccessor.addAll( block,
                               LineLength( static_cast<LineLength::UnderlyingType>( maxLength ) ),
                               linePositions, state.encodingGuess );

        // Update the caller for progress indication
        const auto progress
            = ( state.file_size > 0 ) ? calculateProgress( state.pos, state.file_size ) : 100;

        if ( progress != scopedAccessor.getProgress() ) {
            scopedAccessor.setProgress( progress );
            LOG_INFO << "Indexing progress " << progress << ", indexed size " << state.pos;
            emit indexingProgressed( progress );
        }
    }
    else {
        scopedAccessor.setEncodingGuess( state.encodingGuess );
    }

    LOG_DEBUG << "Indexing block " << blockBeginning << " done";
}

void IndexOperation::doIndex( LineOffset initialPosition )
{
    QFile file( fileName_ );

    if ( !( file.isOpen() || file.open( QIODevice::ReadOnly ) ) ) {
        // TODO: Check that the file is seekable?
        // If the file cannot be open, we do as if it was empty
        LOG_WARNING << "Cannot open file " << fileName_.toStdString();

        IndexingData::MutateAccessor scopedAccessor{ indexing_data_.get() };

        scopedAccessor.clear();
        scopedAccessor.setEncodingGuess( QTextCodec::codecForLocale() );

        scopedAccessor.setProgress( 100 );
        emit indexingProgressed( 100 );
        return;
    }

    IndexingState state;
    state.pos = initialPosition.get();
    state.file_size = file.size();

    {
        IndexingData::ConstAccessor scopedAccessor{ indexing_data_.get() };

        state.fileTextCodec = scopedAccessor.getForcedEncoding();
        if ( !state.fileTextCodec ) {
            state.fileTextCodec = scopedAccessor.getEncodingGuess();
        }

        state.encodingGuess = scopedAccessor.getEncodingGuess();
    }

    const auto& config = Configuration::get();
    const auto prefetchBufferSize = static_cast<size_t>( config.indexReadBufferSizeMb() );

    LOG_INFO << "Prefetch buffer " << readableSize( prefetchBufferSize * IndexingBlockSize );

    using namespace std::chrono;
    using clock = high_resolution_clock;
    microseconds ioDuration{};

    const auto indexingStartTime = clock::now();

    tbb::flow::graph indexingGraph;

    std::thread ioThread;
    auto blockReaderAsync = BlockReader(
        indexingGraph, tbb::flow::serial,
        [ this, &ioThread, &file, &ioDuration ]( const auto&, auto& gateway ) {
            gateway.reserve_wait();
            ioThread = std::thread( [ this, &file, &ioDuration, gw = std::ref( gateway ) ] {
                ioDuration = readFileInBlocks( file, gw.get() );
                gw.get().release_wait();
            } );
        } );

    auto blockPrefetcher = tbb::flow::limiter_node<BlockData>( indexingGraph, prefetchBufferSize );
    auto blockQueue = tbb::flow::queue_node<BlockData>( indexingGraph );

    auto blockParser = tbb::flow::function_node<BlockData, tbb::flow::continue_msg>(
        indexingGraph, tbb::flow::serial, [ this, &state ]( const BlockData& blockData ) {
            indexNextBlock( state, blockData );
            return tbb::flow::continue_msg{};
        } );

    tbb::flow::make_edge( blockReaderAsync, blockPrefetcher );
    tbb::flow::make_edge( blockPrefetcher, blockQueue );
    tbb::flow::make_edge( blockQueue, blockParser );
    tbb::flow::make_edge( blockParser, blockPrefetcher.decrementer() );

    file.seek( state.pos );
    blockReaderAsync.try_put( tbb::flow::continue_msg{} );
    indexingGraph.wait_for_all();
    ioThread.join();

    IndexingData::MutateAccessor scopedAccessor{ indexing_data_.get() };

    LOG_DEBUG << "Indexed up to " << state.pos;

    // Check if there is a non LF terminated line at the end of the file
    if ( !interruptRequest_ && state.file_size > state.pos ) {
        LOG_WARNING << "Non LF terminated file, adding a fake end of line";

        FastLinePositionArray line_position;
        line_position.append( LineOffset( state.file_size + 1 ) );
        line_position.setFakeFinalLF();

        scopedAccessor.addAll( {}, 0_length, line_position, state.encodingGuess );
    }

    const auto endFilePos = file.pos();
    file.reset();
    QByteArray hashBuffer( IndexingBlockSize, Qt::Uninitialized );
    const auto headerHashSize = file.read( hashBuffer.data(), hashBuffer.size() );
    FileDigest fastHashDigest;
    fastHashDigest.addData( hashBuffer.data(), static_cast<size_t>( headerHashSize ) );

    scopedAccessor.setHeaderHash( fastHashDigest.digest(), headerHashSize );

    if ( endFilePos <= hashBuffer.size() ) {
        scopedAccessor.setTailHash( fastHashDigest.digest(), 0, headerHashSize );
    }
    else {
        const auto tailHashOffset = endFilePos - hashBuffer.size();
        file.seek( tailHashOffset );
        const auto tailHashSize = file.read( hashBuffer.data(), hashBuffer.size() );
        fastHashDigest.reset();
        fastHashDigest.addData( hashBuffer.data(), static_cast<size_t>( tailHashSize ) );
        scopedAccessor.setTailHash( fastHashDigest.digest(), tailHashOffset, tailHashSize );
    }

    const auto indexingEndTime = high_resolution_clock::now();
    const auto duration = duration_cast<microseconds>( indexingEndTime - indexingStartTime );

    LOG_INFO << "Indexing done, took " << duration << ", io " << ioDuration;
    LOG_INFO << "Index size "
             << readableSize( static_cast<uint64_t>( scopedAccessor.allocatedSize() ) );
    LOG_INFO << "Indexed lines " << scopedAccessor.getNbLines();
    LOG_INFO << "Max line " << scopedAccessor.getMaxLength();
    LOG_INFO << "Indexing perf "
             << ( 1000.f * 1000.f * static_cast<float>( state.file_size )
                  / static_cast<float>( duration.count() ) )
                    / ( 1024 * 1024 )
             << " MiB/s";

    if ( interruptRequest_ ) {
        scopedAccessor.clear();
    }

    if ( scopedAccessor.getMaxLength().get()
         == std::numeric_limits<LineLength::UnderlyingType>::max() ) {

        QMessageBox::critical( nullptr, "Klogg", "Can't index file: some lines are too long",
                               QMessageBox::Abort );

        scopedAccessor.clear();
    }

    if ( !scopedAccessor.getEncodingGuess() ) {
        scopedAccessor.setEncodingGuess( QTextCodec::codecForLocale() );
    }
}

// Called in the worker thread's context
OperationResult FullIndexOperation::run()
{
    try {
        LOG_INFO << "FullIndexOperation::run(), file " << fileName_.toStdString();

        emit indexingProgressed( 0 );

        {
            IndexingData::MutateAccessor scopedAccessor{ indexing_data_.get() };
            scopedAccessor.clear();
            scopedAccessor.forceEncoding( forcedEncoding_ );
        }

        doIndex( 0_offset );

        LOG_INFO << "FullIndexOperation: ... finished, interrupt = "
                 << static_cast<bool>( interruptRequest_ );

        const auto result = interruptRequest_ ? false : true;
        emit indexingFinished( result );
        return result;
    } catch ( const std::exception& err ) {
        const auto errorString = QString( "FullIndexOperation failed: %1" ).arg( err.what() );
        LOG_ERROR << errorString;
        dispatchToMainThread( [ errorString ]() {
            IssueReporter::askUserAndReportIssue( IssueTemplate::Exception, errorString );
        } );

        IndexingData::MutateAccessor scopedAccessor{ indexing_data_.get() };
        scopedAccessor.clear();
        return false;
    }
}

OperationResult PartialIndexOperation::run()
{
    try {
        LOG_INFO << "PartialIndexOperation::run(), file " << fileName_.toStdString();

        const auto initialPosition
            = LineOffset( IndexingData::ConstAccessor{ indexing_data_.get() }.getIndexedSize() );

        LOG_INFO << "PartialIndexOperation: Starting the count at " << initialPosition << " ...";

        emit indexingProgressed( 0 );

        doIndex( initialPosition );

        LOG_INFO << "PartialIndexOperation: ... finished counting.";

        const auto result = interruptRequest_ ? false : true;
        emit indexingFinished( result );
        return result;
    } catch ( const std::exception& err ) {
        const auto errorString = QString( "PartialIndexOperation failed: %1" ).arg( err.what() );
        LOG_ERROR << errorString;
        dispatchToMainThread( [ errorString ]() {
            IssueReporter::askUserAndReportIssue( IssueTemplate::Exception, errorString );
        } );

        IndexingData::MutateAccessor scopedAccessor{ indexing_data_.get() };
        scopedAccessor.clear();

        return false;
    }
}

OperationResult CheckFileChangesOperation::run()
{
    try {
        LOG_INFO << "CheckFileChangesOperation::run(), file " << fileName_.toStdString();
        const auto result = doCheckFileChanges();
        emit fileCheckFinished( result );
        return result;
    } catch ( const std::exception& err ) {
        const auto errorString
            = QString( "CheckFileChangesOperation failed: %1" ).arg( err.what() );
        LOG_ERROR << errorString;
        dispatchToMainThread( [ errorString ]() {
            IssueReporter::askUserAndReportIssue( IssueTemplate::Exception, errorString );
        } );
        return MonitoredFileStatus::Truncated;
    }
}

MonitoredFileStatus CheckFileChangesOperation::doCheckFileChanges()
{
    QFileInfo info( fileName_ );
    const auto indexedHash = IndexingData::ConstAccessor{ indexing_data_.get() }.getHash();
    const auto realFileSize = info.size();

    if ( realFileSize == 0 || realFileSize < indexedHash.size ) {
        LOG_INFO << "File truncated";
        return MonitoredFileStatus::Truncated;
    }
    else {
        QFile file( fileName_ );

        QByteArray buffer{ IndexingBlockSize, Qt::Uninitialized };

        bool isFileModified = false;
        const auto& config = Configuration::get();

        if ( !file.isOpen() && !file.open( QIODevice::ReadOnly ) ) {
            LOG_INFO << "File failed to open";
            return MonitoredFileStatus::Truncated;
        }

        const auto getDigest = [ &file, &buffer ]( const qint64 indexedSize ) {
            FileDigest fileDigest;
            auto readSize = 0ll;
            auto totalSize = 0ll;
            do {
                const auto bytesToRead
                    = std::min( static_cast<qint64>( buffer.size() ), indexedSize - totalSize );
                readSize = file.read( buffer.data(), bytesToRead );

                if ( readSize > 0 ) {
                    fileDigest.addData( buffer.data(), static_cast<size_t>( readSize ) );
                    totalSize += readSize;
                }

            } while ( readSize > 0 && totalSize < indexedSize );

            return fileDigest.digest();
        };
        if ( config.fastModificationDetection() ) {
            const auto headerDigest = getDigest( indexedHash.headerSize );

            LOG_INFO << "indexed header xxhash " << indexedHash.headerDigest;
            LOG_INFO << "current header xxhash " << headerDigest << ", size "
                     << indexedHash.headerSize;

            isFileModified = headerDigest != indexedHash.headerDigest;

            if ( !isFileModified ) {
                file.seek( indexedHash.tailOffset );
                const auto tailDigest = getDigest( indexedHash.tailSize );

                LOG_INFO << "indexed tail xxhash " << indexedHash.tailDigest;
                LOG_INFO << "current tail xxhash " << tailDigest << ", size "
                         << indexedHash.tailSize;

                isFileModified = tailDigest != indexedHash.tailDigest;
            }
        }
        else {

            const auto realHashDigest = getDigest( indexedHash.size );

            LOG_INFO << "indexed xxhash " << indexedHash.fullDigest;
            LOG_INFO << "current xxhash " << realHashDigest;

            isFileModified = realHashDigest != indexedHash.fullDigest;
        }

        if ( isFileModified ) {
            LOG_INFO << "File changed in indexed range";
            return MonitoredFileStatus::Truncated;
        }
        else if ( realFileSize > indexedHash.size ) {
            LOG_INFO << "New data on disk";
            return MonitoredFileStatus::DataAdded;
        }
        else {
            LOG_INFO << "No change in file";
            return MonitoredFileStatus::Unchanged;
        }
    }
}
