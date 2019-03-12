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

#include <QObject>
#include <QString>
#include <QFile>
#include <QVector>
#include <QMutex>
#include <QDateTime>
#include <QTextCodec>

#include "abstractlogdata.h"
#include "logdataworkerthread.h"
#include "filewatcher.h"
#include "loadingstatus.h"

#include "fileid.h"

class LogFilteredData;

// Thrown when trying to attach an already attached LogData
class CantReattachErr {};

// Represents a complete set of data to be displayed (ie. a log file content)
// This class is thread-safe.
class LogData : public AbstractLogData {
  Q_OBJECT

  public:
    // Creates an empty LogData
    LogData();
    // Destroy an object
    ~LogData() override;

    enum MonitoredFileStatus { Unchanged, DataAdded, Truncated };

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
    LogFilteredData* getNewFilteredData() const;
    // Returns the size if the file in bytes
    qint64 getFileSize() const;
    // Returns the last modification date for the file.
    // Null if the file is not on disk.
    QDateTime getLastModifiedDate() const;
    // Throw away all the file data and reload/reindex.
    void reload(QTextCodec* forcedEncoding = nullptr);

    // Get the auto-detected encoding for the indexed text.
    QTextCodec* getDetectedEncoding() const;

  signals:
    // Sent during the 'attach' process to signal progress
    // percent being the percentage of completion.
    void loadingProgressed( int percent );
    // Signal the client the file is fully loaded and available.
    void loadingFinished( LoadingStatus status );
    // Sent when the file on disk has changed, will be followed
    // by loadingProgressed if needed and then a loadingFinished.
    void fileChanged( LogData::MonitoredFileStatus status );

  private slots:
    // Consider reloading the file when it changes on disk updated
    void fileChangedOnDisk(const QString &filename);
    // Called when the worker thread signals the current operation ended
    void indexingFinished( LoadingStatus status );

  private:
    // This class models an indexing operation.
    // It exists to permit LogData to delay the operation if another
    // one is ongoing (operations are asynchronous)
    class LogDataOperation {
      public:
        LogDataOperation() = default;
        explicit LogDataOperation( const QString& fileName ) : filename_( fileName ) {}

        // Permit each child to have its destructor
        virtual ~LogDataOperation() = default;

        void start( LogDataWorkerThread& workerThread ) const
        { doStart( workerThread ); }
        const QString& getFilename() const { return filename_; }

      protected:
        virtual void doStart( LogDataWorkerThread& workerThread ) const = 0;
        QString filename_;
    };

    // Attaching a new file (change name + full index)
    class AttachOperation : public LogDataOperation {
      public:
        AttachOperation( const QString& fileName )
            : LogDataOperation( fileName ) {}

      protected:
        void doStart( LogDataWorkerThread& workerThread ) const override;
    };

    // Reindexing the current file
    class FullIndexOperation : public LogDataOperation {
      public:
        explicit FullIndexOperation( QTextCodec* forcedEncoding = nullptr )
             : forcedEncoding_( forcedEncoding )
        {}

      protected:
        void doStart( LogDataWorkerThread& workerThread ) const override;

      private:
        QTextCodec* forcedEncoding_;
    };

    // Indexing part of the current file (from fileSize)
    class PartialIndexOperation : public LogDataOperation {
      protected:
        void doStart( LogDataWorkerThread& workerThread ) const override;
    };

    MonitoredFileStatus fileChangedOnDisk_;

    // Implementation of virtual functions
    QString doGetLineString( LineNumber line ) const override;
    QString doGetExpandedLineString( LineNumber line ) const override;
    std::vector<QString> doGetLines( LineNumber first, LinesCount number ) const override;
    std::vector<QString> doGetExpandedLines( LineNumber first, LinesCount number ) const override;
    LinesCount doGetNbLine() const override;
    LineLength doGetMaxLength() const override;
    LineLength doGetLineLength(LineNumber line ) const override;
    void doSetDisplayEncoding( const char* encoding ) override;
    QTextCodec* doGetDisplayEncoding() const override;

    void enqueueOperation( std::shared_ptr<const LogDataOperation> newOperation );
    void startOperation();
    void reOpenFile() const;

    QString indexingFileName_;
    mutable std::unique_ptr<QFile> attached_file_;
    mutable FileId attached_file_id_;

    bool keepFileClosed_;

    // Indexing data, read by us, written by the worker thread
    IndexingData indexing_data_;

    QDateTime lastModifiedDate_;
    std::shared_ptr<const LogDataOperation> currentOperation_;
    std::shared_ptr<const LogDataOperation> nextOperation_;

    // Codec to decode text
    QTextCodec* codec_;

    // Offset to apply to the newline character
    int before_cr_offset_ = 0;
    int after_cr_offset_  = 0;

    // To protect the file:
    mutable QMutex fileMutex_;
    // (are mutable to allow 'const' function to touch it,
    // while remaining const)
    // When acquiring both, data should be help before locking file.

    LogDataWorkerThread workerThread_;
};

Q_DECLARE_METATYPE( LogData::MonitoredFileStatus )

#endif
