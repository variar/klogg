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

#ifndef LOGDATA_H
#define LOGDATA_H

#include <memory>

#include <QDateTime>
#include <QFile>
#include <QObject>
#include <QString>
#include <QTextCodec>
#include <qregularexpression.h>
#include <qtextcodec.h>
#include <string_view>

#include "abstractlogdata.h"
#include "fileholder.h"
#include "filewatcher.h"
#include "loadingstatus.h"
#include "logdataoperation.h"
#include "logdataworker.h"

class LogFilteredData;

// Thrown when trying to attach an already attached LogData
class CantReattachErr {
};

// Represents a complete set of data to be displayed (ie. a log file content)
// This class is thread-safe.
class LogData : public AbstractLogData {
    Q_OBJECT

  public:
    LogData();
    ~LogData();

    LogData( const LogData& ) = delete;
    LogData& operator=( const LogData&& ) = delete;

    LogData( LogData&& ) = delete;
    LogData& operator=( LogData&& ) = delete;

    // Attaches the LogData to a file on disk
    // It starts the asynchronous indexing and returns (almost) immediately
    // Attaching to a non existant file works and the file is reported
    // to be empty.
    // Reattaching is forbidden and will throw.
    void attachFile( const QString& fileName );
    // Interrupt the loading and report a null file.
    // Does nothing if no loading in progress.
    void interruptLoading();
    // Creates a new filtered data.
    // ownership is passed to the caller
    std::unique_ptr<LogFilteredData> getNewFilteredData() const;
    // Returns the size if the file in bytes
    qint64 getFileSize() const;
    // Returns the last modification date for the file.
    // Null if the file is not on disk.
    QDateTime getLastModifiedDate() const;
    // Throw away all the file data and reload/reindex.
    void reload( QTextCodec* forcedEncoding = nullptr );

    // Get the auto-detected encoding for the indexed text.
    QTextCodec* getDetectedEncoding() const;

    void setPrefilter(const QString& prefilterPattern);

    struct RawLines {
        LineNumber startLine;

        std::vector<char> buffer;
        std::vector<qint64> endOfLines;

        TextDecoder textDecoder;

        QRegularExpression prefilterPattern;

      public:
        std::vector<QString> decodeLines() const;
        std::vector<std::string_view> buildUtf8View() const;

      private:
        mutable QByteArray utf8Data_;
    };

    RawLines getLinesRaw( LineNumber first, LinesCount number ) const;

  Q_SIGNALS:
    // Sent during the 'attach' process to signal progress
    // percent being the percentage of completion.
    void loadingProgressed( int percent );
    // Signal the client the file is fully loaded and available.
    void loadingFinished( LoadingStatus status );
    // Sent when the file on disk has changed, will be followed
    // by loadingProgressed if needed and then a loadingFinished.
    void fileChanged( MonitoredFileStatus status );

  private Q_SLOTS:
    // Consider reloading the file when it changes on disk updated
    void fileChangedOnDisk( const QString& filename );
    // Called when the worker thread signals the current operation ended
    void indexingFinished( LoadingStatus status );
    // Called when the worker thread signals the current operation ended
    void checkFileChangesFinished( MonitoredFileStatus status );

  private:
    // Implementation of virtual functions
    QString doGetLineString( LineNumber line ) const override;
    QString doGetExpandedLineString( LineNumber line ) const override;
    std::vector<QString> doGetLines( LineNumber first, LinesCount number ) const override;
    std::vector<QString> doGetExpandedLines( LineNumber first, LinesCount number ) const override;
    LineNumber doGetLineNumber( LineNumber index ) const override;
    LinesCount doGetNbLine() const override;
    LineLength doGetMaxLength() const override;
    LineLength doGetLineLength( LineNumber line ) const override;
    void doSetDisplayEncoding( const char* encoding ) override;
    QTextCodec* doGetDisplayEncoding() const override;
    void doAttachReader() const override;
    void doDetachReader() const override;

    void reOpenFile() const;

    std::vector<QString> getLinesFromFile( LineNumber first, LinesCount number,
                                           QString ( *processLine )( QString&& ) ) const;

  private:
    mutable std::unique_ptr<FileHolder> attached_file_;

    // Indexing data, read by us, written by the worker thread
    std::shared_ptr<IndexingData> indexing_data_;

    OperationQueue operationQueue_;

    QString indexingFileName_;
    // mutable std::unique_ptr<QFile> attached_file_;
    // mutable FileId attached_file_id_;

    bool keepFileClosed_;

    QDateTime lastModifiedDate_;

    // Codec to decode text
    TextCodecHolder codec_;
    MonitoredFileStatus fileChangedOnDisk_;

    QString prefilterPattern_;
};

#endif
