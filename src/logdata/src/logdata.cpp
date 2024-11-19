/*
 * Copyright (C) 2009, 2010, 2013, 2014, 2015 Nicolas Bonnefon and other contributors
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

// This file implements LogData, the content of a log file.

#include <algorithm>
#include <limits>
#include <numeric>
#include <qregularexpression.h>
#include <qtextcodec.h>
#include <string_view>
#include <utility>
#include <vector>

#include <QFileInfo>

#include <simdutf.h>

#include "configuration.h"
#include "containers.h"
#include "linetypes.h"
#include "log.h"
#include "logfiltereddata.h"

#include "logdata.h"

LogData::LogData()
    : AbstractLogData()
    , indexing_data_( std::make_shared<IndexingData>() )
    , operationQueue_( [ this ] { attached_file_->attachReader(); } )
    , codec_( QTextCodec::codecForName( "ISO-8859-1" ) )
{
    // Initialise the file watcher
    connect( &FileWatcher::getFileWatcher(), &FileWatcher::fileChanged, this,
             &LogData::fileChangedOnDisk, Qt::QueuedConnection );

    auto worker = std::make_unique<LogDataWorker>( indexing_data_ );

    // Forward the update signal
    connect( worker.get(), &LogDataWorker::indexingProgressed, this, &LogData::loadingProgressed );
    connect( worker.get(), &LogDataWorker::indexingFinished, this, &LogData::indexingFinished,
             Qt::QueuedConnection );
    connect( worker.get(), &LogDataWorker::checkFileChangesFinished, this,
             &LogData::checkFileChangesFinished, Qt::QueuedConnection );

    operationQueue_.setWorker( std::move( worker ) );

    const auto& config = Configuration::get();
    keepFileClosed_ = config.keepFileClosed();

    if ( keepFileClosed_ ) {
        LOG_INFO << "Keep file closed option is set";
    }

    const auto defaultEncodingMib = config.defaultEncodingMib();
    if ( defaultEncodingMib >= 0 ) {
        codec_.setCodec( QTextCodec::codecForMib( defaultEncodingMib ) );
    }
}

LogData::~LogData()
{
    LOG_DEBUG << "Destroying log data";
    operationQueue_.shutdown();
}

void LogData::setPrefilter( const QString& prefilterPattern )
{
    IndexingData::MutateAccessor scopedAccessor{ indexing_data_.get() };
    prefilterPattern_ = prefilterPattern;
}

void LogData::attachFile( const QString& fileName )
{
    LOG_DEBUG << "LogData::attachFile " << fileName.toStdString();

    if ( attached_file_ ) {
        // We cannot reattach
        throw CantReattachErr();
    }

    indexingFileName_ = fileName;
    attached_file_.reset( new FileHolder( keepFileClosed_ ) );
    attached_file_->open( indexingFileName_ );

    operationQueue_.enqueueOperation<AttachOperation>( fileName );
}

void LogData::interruptLoading()
{
    operationQueue_.interrupt();
}

qint64 LogData::getFileSize() const
{
    return IndexingData::ConstAccessor{ indexing_data_.get() }.getIndexedSize();
}

QDateTime LogData::getLastModifiedDate() const
{
    return lastModifiedDate_;
}

// Return an initialised LogFilteredData. The search is not started.
std::unique_ptr<LogFilteredData> LogData::getNewFilteredData() const
{
    return std::make_unique<LogFilteredData>( this );
}

void LogData::reload( QTextCodec* forcedEncoding )
{
    operationQueue_.interrupt();

    // Re-open the file, useful in case the file has been moved
    attached_file_->reOpenFile();

    operationQueue_.enqueueOperation<FullReindexOperation>( forcedEncoding );
}

void LogData::fileChangedOnDisk( const QString& filename )
{
    LOG_INFO << "signalFileChanged " << filename << ", indexed file " << indexingFileName_;

    QFileInfo info( indexingFileName_ );
    const auto currentFileId = FileId::getFileId( indexingFileName_ );
    const auto attachedFileId = attached_file_->getFileId();

    const auto indexedHash = IndexingData::ConstAccessor{ indexing_data_.get() }.getHash();

    LOG_INFO << "current indexed fileSize=" << indexedHash.size;
    LOG_INFO << "current indexed hash=" << indexedHash.fullDigest;
    LOG_INFO << "info file_->size()=" << info.size();

    LOG_INFO << "attached_file_->size()=" << attached_file_->size();
    LOG_INFO << "attached_file_id_ index " << attachedFileId.fileIndex;
    LOG_INFO << "currentFileId index " << currentFileId.fileIndex;

    // In absence of any clearer information, we use the following size comparison
    // to determine whether we are following the same file or not (i.e. the file
    // has been moved and the inode we are following is now under a new name, if for
    // instance log has been rotated). We want to follow the name so we have to reopen
    // the file to ensure we are reading the right one.
    // This is a crude heuristic but necessary for notification services that do not
    // give details (e.g. kqueues)

    const bool isFileIdChanged = attachedFileId != currentFileId;

    if ( !isFileIdChanged && filename != indexingFileName_ ) {
        LOG_INFO << "ignore other file update";
        return;
    }

    if ( isFileIdChanged || ( info.size() != attached_file_->size() )
         || ( !attached_file_->isOpen() ) ) {

        LOG_INFO << "Inconsistent size, or file index, the file might have changed, re-opening";

        attached_file_->reOpenFile();
    }

    operationQueue_.enqueueOperation<CheckDataChangesOperation>();
}

void LogData::indexingFinished( LoadingStatus status )
{
    attached_file_->detachReader();

    LOG_INFO << "indexingFinished for: " << indexingFileName_
             << ( status == LoadingStatus::Successful ) << ", found "
             << IndexingData::ConstAccessor{ indexing_data_.get() }.getNbLines() << " lines.";

    if ( status == LoadingStatus::Successful ) {
        FileWatcher::getFileWatcher().addFile( indexingFileName_ );

        // Update the modified date/time if the file exists
        lastModifiedDate_ = QDateTime();
        QFileInfo fileInfo( indexingFileName_ );
        if ( fileInfo.exists() )
            lastModifiedDate_ = fileInfo.lastModified();
    }

    fileChangedOnDisk_ = MonitoredFileStatus::Unchanged;

    LOG_DEBUG << "Sending indexingFinished.";
    Q_EMIT loadingFinished( status );

    operationQueue_.finishOperationAndStartNext();
}

void LogData::checkFileChangesFinished( MonitoredFileStatus status )
{
    attached_file_->detachReader();

    LOG_INFO << "File " << indexingFileName_ << " status " << static_cast<uint8_t>( status );

    if ( fileChangedOnDisk_ != MonitoredFileStatus::Truncated ) {
        switch ( status ) {
        case MonitoredFileStatus::Truncated:
            fileChangedOnDisk_ = MonitoredFileStatus::Truncated;
            operationQueue_.enqueueOperation<FullReindexOperation>();
            break;
        case MonitoredFileStatus::DataAdded:
            fileChangedOnDisk_ = MonitoredFileStatus::DataAdded;
            operationQueue_.enqueueOperation<PartialReindexOperation>();
            break;
        case MonitoredFileStatus::Unchanged:
            fileChangedOnDisk_ = MonitoredFileStatus::Unchanged;
            break;
        }
    }
    else {
        operationQueue_.enqueueOperation<FullReindexOperation>();
    }

    if ( status != MonitoredFileStatus::Unchanged
         || fileChangedOnDisk_ == MonitoredFileStatus::Truncated ) {
        Q_EMIT fileChanged( fileChangedOnDisk_ );
    }

    operationQueue_.finishOperationAndStartNext();
}

//
// Implementation of virtual functions
//
LinesCount LogData::doGetNbLine() const
{
    return IndexingData::ConstAccessor{ indexing_data_.get() }.getNbLines();
}

LineLength LogData::doGetMaxLength() const
{
    return IndexingData::ConstAccessor{ indexing_data_.get() }.getMaxLength();
}

LineLength LogData::doGetLineLength( LineNumber line ) const
{
    if ( line >= IndexingData::ConstAccessor{ indexing_data_.get() }.getNbLines() ) {
        return 0_length; /* exception? */
    }

    return LineLength{ doGetExpandedLineString( line ).size() };
}

void LogData::doSetDisplayEncoding( const char* encoding )
{
    LOG_DEBUG << "AbstractLogData::setDisplayEncoding: " << encoding;
    codec_.setCodec( QTextCodec::codecForName( encoding ) );
    auto needReload = false;
    auto useGuessedCodec = false;

    {
        IndexingData::ConstAccessor scopedAccessor{ indexing_data_.get() };

        const QTextCodec* currentIndexCodec = scopedAccessor.getForcedEncoding();
        if ( !currentIndexCodec ) {
            currentIndexCodec = scopedAccessor.getEncodingGuess();
        }

        if ( currentIndexCodec && codec_.mibEnum() != currentIndexCodec->mibEnum() ) {
            if ( codec_.encodingParameters() != EncodingParameters( currentIndexCodec ) ) {
                needReload = true;
                useGuessedCodec = codec_.mibEnum() == scopedAccessor.getEncodingGuess()->mibEnum();
            }
        }
    }

    if ( needReload ) {
        reload( useGuessedCodec ? nullptr : codec_.codec() );
    }
}

QTextCodec* LogData::doGetDisplayEncoding() const
{
    return codec_.codec();
}

OneLineLog LogData::doGetOneLineLog( LineNumber line ) const
{
    auto lines = doGetOneLineLogs( line, 1_lcount );
    return lines.empty() ? OneLineLog{} : std::move( lines.front() );
}

klogg::vector<OneLineLog> LogData::doGetOneLineLogs( LineNumber firstLine, LinesCount number ) const
{
    LOG_DEBUG << "firstLine:" << firstLine << " nb:" << number;

    if ( number.get() == 0 ) {
        return {};
    }

    klogg::vector<OneLineLog> processedLines;
    std::shared_ptr<TextDecoder> dec{ std::make_shared<TextDecoder>( codec_.makeDecoder() ) };
    std::shared_ptr<QRegularExpression> reg{ std::make_shared<QRegularExpression>(
        prefilterPattern_, QRegularExpression::CaseInsensitiveOption ) };
    try {
        const auto rawLines = getLinesRaw( firstLine, number );
        processedLines = rawLines.splitLines(
            [ &dec, &reg ]( const char* log, OneLineLog::Length len ) -> OneLineLog {
                return { log, len, dec, reg };
            } );

    } catch ( const std::bad_alloc& e ) {
        LOG_ERROR << "not enough memory " << e.what();
        const char* replaceLog = "KLOGG WARNING: not enough memory";
        processedLines.emplace_back(
            replaceLog, type_safe::narrow_cast<OneLineLog::Length>( ::strlen( replaceLog ) ), dec,
            reg );
    }

    while ( processedLines.size() < number.get() ) {
        const char* replaceLog = "KLOGG WARNING: failed to read some lines before this one";
        processedLines.emplace_back(
            replaceLog, type_safe::narrow_cast<OneLineLog::Length>( ::strlen( replaceLog ) ), dec,
            reg );
    }

    return processedLines;
}

QString LogData::doGetLineString( LineNumber line ) const
{
    const auto lines = doGetLines( line, 1_lcount );
    return lines.empty() ? QString{} : lines.front();
}

QString LogData::doGetExpandedLineString( LineNumber line ) const
{
    return untabify( doGetLineString( line ) );
}

// Note this function is also called from the LogFilteredDataWorker thread, so
// data must be protected because they are changed in the main thread (by
// indexingFinished).
klogg::vector<QString> LogData::doGetLines( LineNumber first_line, LinesCount number ) const
{
    return getLinesFromFile( first_line, number, []( QString&& lineData ) {
        if ( lineData.endsWith( QChar::CarriageReturn ) ) {
            lineData.chop( 1 );
        }
        return std::move( lineData );
    } );
}

klogg::vector<QString> LogData::doGetExpandedLines( LineNumber first_line, LinesCount number ) const
{
    return getLinesFromFile( first_line, number, []( QString&& lineData ) {
        return untabify( std::move( lineData ) );
    } );
}

LineNumber LogData::doGetLineNumber( LineNumber index ) const
{
    return index;
}

LogData::RawLines LogData::getLinesRaw( LineNumber firstLine, LinesCount number ) const
{
    RawLines rawLines;
    rawLines.startLine = firstLine;

    try {
        IndexingData::ConstAccessor scopedAccessor{ indexing_data_.get() };
        if ( (firstLine + number).get() > scopedAccessor.getNbLines().get() ) {
            LOG_WARNING << "Lines out of bound asked for";
            return {}; /* exception? */
        }

        rawLines.endOfLines.reserve( number.get() );
        rawLines.prefilterPattern
            = !prefilterPattern_.isEmpty()
                  ? QRegularExpression( prefilterPattern_,
                                        QRegularExpression::CaseInsensitiveOption )
                  : QRegularExpression{};

        ScopedFileHolder<FileHolder> fileHolder( attached_file_.get() );

        const auto firstByte
            = ( firstLine == 0_lnum )
                  ? 0
                  : scopedAccessor.getEndOfLineOffset( firstLine - 1_lcount ).get();

        klogg::vector<OffsetInFile> endOfLines
            = scopedAccessor.getEndOfLineOffsets( firstLine, number );

        const auto lastByte = endOfLines.back().get();

        std::transform(
            endOfLines.begin(), endOfLines.end(), std::back_inserter( rawLines.endOfLines ),
            [ firstByte ]( const OffsetInFile& offset ) { return offset.get() - firstByte; } );

        const auto bytesToRead = lastByte - firstByte;
        LOG_DEBUG << "will try to read:" << bytesToRead << " bytes";
        rawLines.buffer.resize( static_cast<std::size_t>( bytesToRead ) );

        fileHolder.getFile()->seek( firstByte );
        const auto bytesRead = fileHolder.getFile()->read( rawLines.buffer.data(), bytesToRead );

        if ( bytesRead != bytesToRead ) {
            LOG_DEBUG << "failed to read " << bytesToRead << " bytes, got " << bytesRead;
        }

        LOG_DEBUG << "done reading lines:" << rawLines.buffer.size();
        rawLines.textDecoder = codec_.makeDecoder();
        return rawLines;

    } catch ( const std::bad_alloc& ) {
        LOG_ERROR << "not enough memory";
        rawLines.endOfLines.clear();
        rawLines.buffer.clear();
        return rawLines;
    }
}

klogg::vector<QString> LogData::getLinesFromFile( LineNumber firstLine, LinesCount number,
                                                  QString ( *processLine )( QString&& ) ) const
{
    LOG_DEBUG << "firstLine:" << firstLine << " nb:" << number;

    if ( number.get() == 0 ) {
        return klogg::vector<QString>();
    }

    klogg::vector<QString> processedLines;
    try {
        const auto rawLines = getLinesRaw( firstLine, number );
        auto decodedLines = rawLines.decodeLines();

        processedLines.reserve( decodedLines.size() );

        for ( auto&& line : decodedLines ) {
            processedLines.push_back( processLine( std::move( line ) ) );
        }

    } catch ( const std::bad_alloc& e ) {
        LOG_ERROR << "not enough memory " << e.what();
        processedLines.emplace_back( "KLOGG WARNING: not enough memory" );
    }

    processedLines.reserve( number.get() - processedLines.size() );
    while ( processedLines.size() < number.get() ) {
        processedLines.emplace_back( "KLOGG WARNING: failed to read some lines before this one" );
    }

    return processedLines;
}

QTextCodec* LogData::getDetectedEncoding() const
{
    return IndexingData::ConstAccessor{ indexing_data_.get() }.getEncodingGuess();
}

void LogData::doAttachReader() const
{
    attached_file_->attachReader();
}

void LogData::doDetachReader() const
{
    attached_file_->detachReader();
}

klogg::vector<QString> LogData::RawLines::decodeLines() const
{
    if ( this->endOfLines.empty() ) {
        return klogg::vector<QString>();
    }

    klogg::vector<QString> decodedLines;
    decodedLines.reserve( this->endOfLines.size() );

    try {
        qint64 lineStart = 0;
        size_t currentLineIndex = 0;
        const auto lineFeedWidth = textDecoder.encodingParams.lineFeedWidth;
        for ( const auto& lineEnd : this->endOfLines ) {
            const auto length = lineEnd - lineStart - lineFeedWidth;
            LOG_DEBUG << "line " << this->startLine.get() + currentLineIndex << ", length "
                      << length;

            constexpr auto maxlength = std::numeric_limits<int>::max() / 2;
            if ( length >= maxlength ) {
                decodedLines.emplace_back( "KLOGG WARNING: this line is too long" );
                break;
            }

            if ( lineStart + length > klogg::ssize( buffer ) ) {
                decodedLines.emplace_back( "KLOGG WARNING: file read failed" );
                LOG_WARNING << "not enough data in buffer";
                break;
            }

            auto decodedLine = textDecoder.decoder->toUnicode(
                buffer.data() + lineStart, type_safe::narrow_cast<int>( length ) );

            if ( !prefilterPattern.pattern().isEmpty() ) {
                decodedLine.remove( prefilterPattern );
            }

            decodedLines.push_back( std::move( decodedLine ) );

            lineStart = lineEnd;
        }
    } catch ( const std::bad_alloc& ) {
        LOG_ERROR << "not enough memory";
        decodedLines.emplace_back( "KLOGG WARNING: not enough memory" );
    }

    decodedLines.reserve( this->endOfLines.size() - decodedLines.size() );
    while ( decodedLines.size() < this->endOfLines.size() ) {
        decodedLines.emplace_back( "KLOGG WARNING: failed to decode some lines before this one" );
    }

    return decodedLines;
}

klogg::vector<OneLineLog>
LogData::RawLines::splitLines( OneLineLogConstructor makeOneLineLog ) const
{
    if ( this->endOfLines.empty() ) {
        return {};
    }

    klogg::vector<OneLineLog> decodedLines;
    decodedLines.reserve( this->endOfLines.size() );

    try {
        qint64 lineStart = 0;
        //        size_t currentLineIndex = 0;
        const auto lineFeedWidth = textDecoder.encodingParams.lineFeedWidth;
        for ( const auto& lineEnd : this->endOfLines ) {
            const auto length = lineEnd - lineStart - lineFeedWidth;

            constexpr auto maxlength = std::numeric_limits<int>::max() / 2;
            if ( length >= maxlength ) {
                const char* log = "KLOGG WARNING: this line is too long";
                decodedLines.emplace_back( makeOneLineLog(
                    log, type_safe::narrow_cast<OneLineLog::Length>( ::strlen( log ) ) ) );
                break;
            }

            if ( lineStart + length > klogg::ssize( buffer ) ) {
                const char* log = "KLOGG WARNING: file read failed";
                decodedLines.emplace_back( makeOneLineLog(
                    log, type_safe::narrow_cast<OneLineLog::Length>( ::strlen( log ) ) ) );
                LOG_WARNING << "not enough data in buffer";
                break;
            }

            decodedLines.emplace_back( makeOneLineLog(
                buffer.data() + lineStart, type_safe::narrow_cast<OneLineLog::Length>( length ) ) );

            lineStart = lineEnd;
        }
    } catch ( const std::bad_alloc& ) {
        LOG_ERROR << "not enough memory";
        const char* log = "KLOGG WARNING: not enough memory";
        decodedLines.emplace_back(
            makeOneLineLog( log, type_safe::narrow_cast<OneLineLog::Length>( ::strlen( log ) ) ) );
    }

    while ( decodedLines.size() < this->endOfLines.size() ) {
        const char* log = "KLOGG WARNING: failed to decode some lines before this one";
        decodedLines.emplace_back(
            makeOneLineLog( log, type_safe::narrow_cast<OneLineLog::Length>( ::strlen( log ) ) ) );
    }

    return decodedLines;
}

klogg::vector<std::string_view> LogData::RawLines::buildUtf8View() const
{
    klogg::vector<std::string_view> lines;
    if ( this->endOfLines.empty() || textDecoder.decoder == nullptr ) {
        return lines;
    }

    try {
        // const auto optimizeForNotLatinEncodings
        //     = Configuration::get().optimizeForNotLatinEncodings();

        lines.reserve( endOfLines.size() );

        std::string_view wholeString;

        if ( prefilterPattern.pattern().isEmpty() && textDecoder.encodingParams.isUtf8Compatible ) {
            wholeString = std::string_view( buffer.data(), buffer.size() );
        }
        else {

            QString utf16Data;
            if ( prefilterPattern.pattern().isEmpty() && textDecoder.encodingParams.isUtf16LE ) {
                utf16Data = QString::fromRawData( reinterpret_cast<const QChar*>( buffer.data() ),
                                                  klogg::isize( buffer ) / 2 );
            }
            else {
                utf16Data = textDecoder.decoder->toUnicode( buffer.data(), klogg::isize( buffer ) );
            }

            if ( !prefilterPattern.pattern().isEmpty() ) {
                utf16Data.remove( prefilterPattern );
            }

            size_t resultSize = 0;
            // if ( !optimizeForNotLatinEncodings ) {
            //     utf8Data_ = utf16Data.toUtf8();
            //     resultSize = static_cast<size_t>( utf8Data_.size() );
            // }
            // else {
            utf8Data_.resize( buffer.size() * 4 );
            resultSize = simdutf::convert_utf16_to_utf8(
                reinterpret_cast<const char16_t*>( utf16Data.utf16() ),
                static_cast<size_t>( utf16Data.size() ), utf8Data_.data() );
            // }

            wholeString = { utf8Data_.data(), resultSize };
        }

        auto nextLineFeed = wholeString.find( '\n' );
        while ( nextLineFeed != std::string_view::npos ) {
            lines.push_back( wholeString.substr( 0, nextLineFeed ) );
            wholeString.remove_prefix( nextLineFeed + 1 );
            nextLineFeed = wholeString.find( '\n' );
        }

        if ( !wholeString.empty() ) {
            lines.push_back( wholeString );
        }

    } catch ( const std::exception& e ) {
        LOG_ERROR << "failed to transform lines to utf8 " << e.what();
        const auto lastLineOffset = utf8Data_.size();
        // utf8Data_.append( "KLOGG WARNING: not enough memory, try decrease search buffer" );
        lines.reserve( this->endOfLines.size() - lines.size() );
        while ( lines.size() < this->endOfLines.size() ) {
            lines.emplace_back( utf8Data_.data() + lastLineOffset,
                                utf8Data_.size() - lastLineOffset );
        }
    }

    return lines;
}
