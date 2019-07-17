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

#ifndef LOGDATAWORKERTHREAD_H
#define LOGDATAWORKERTHREAD_H

#include <QFuture>
#include <QFutureWatcher>
#include <QMutex>
#include <QObject>
#include <QSemaphore>
#include <QTextCodec>

#include "encodingdetector.h"
#include "linepositionarray.h"
#include "loadingstatus.h"

#include "atomicflag.h"

// This class is a thread-safe set of indexing data.
class IndexingData {
  public:
    IndexingData()
        : dataMutex_()
        , linePosition_()
        , maxLength_( 0 )
        , indexedSize_( 0 )
        , encodingGuess_( QTextCodec::codecForLocale() )
        , encodingForced_( nullptr )
    {
    }

    // Get the total indexed size
    qint64 getSize() const;

    // Get the length of the longest line
    LineLength getMaxLength() const;

    // Get the total number of lines
    LinesCount getNbLines() const;

    // Get the position (in byte from the beginning of the file)
    // of the end of the passed line.
    LineOffset getPosForLine( LineNumber line ) const;

    // Get the guessed encoding for the content.
    QTextCodec* getEncodingGuess() const;

    QTextCodec* getForcedEncoding() const;
    void forceEncoding( QTextCodec* codec );

    // Atomically add to all the existing
    // indexing data.
    void addAll( qint64 size, LineLength length, const FastLinePositionArray& linePosition,
                 QTextCodec* encoding );

    // Completely clear the indexing data.
    void clear();

  private:
    mutable QMutex dataMutex_;

    LinePositionArray linePosition_;
    LineLength maxLength_;
    qint64 indexedSize_;

    QTextCodec* encodingGuess_;
    QTextCodec* encodingForced_;
};

struct IndexingState {
    EncodingParameters encodingParams;
    LineOffset::UnderlyingType pos;
    LineLength::UnderlyingType max_length;
    LineLength::UnderlyingType additional_spaces;
    LineOffset::UnderlyingType end;
    LineOffset::UnderlyingType file_size;

    QTextCodec* encodingGuess;
    QTextCodec* fileTextCodec;

    QSemaphore indexingSem;
    QSemaphore blockSem;
};

class IndexOperation : public QObject {
    Q_OBJECT
  public:
    IndexOperation( const QString& fileName, IndexingData& indexingData,
                    AtomicFlag& interruptRequest )
        : fileName_( fileName )
        , indexing_data_( indexingData )
        , interruptRequest_( interruptRequest )
    {
    }

    // Start the indexing operation, returns true if it has been done
    // and false if it has been cancelled (results not copied)
    virtual bool start() = 0;

  signals:
    void indexingProgressed( int );

  protected:
    // Returns the total size indexed
    // Modify the passed linePosition and maxLength
    void doIndex( LineOffset initialPosition );

    QString fileName_;
    IndexingData& indexing_data_;
    AtomicFlag& interruptRequest_;

  private:
    FastLinePositionArray parseDataBlock( LineOffset::UnderlyingType blockBegining,
                                          const QByteArray& block, IndexingState& state ) const;

    void guessEncoding( const QByteArray& block, IndexingState& state ) const;

    auto setupIndexingProcess( IndexingState& state );
};

class FullIndexOperation : public IndexOperation {
    Q_OBJECT
  public:
    FullIndexOperation( const QString& fileName, IndexingData& indexingData,
                        AtomicFlag& interruptRequest, QTextCodec* forcedEncoding = nullptr )
        : IndexOperation( fileName, indexingData, interruptRequest )
        , forcedEncoding_( forcedEncoding )
    {
    }
    bool start() override;

  private:
    QTextCodec* forcedEncoding_;
};

class PartialIndexOperation : public IndexOperation {
    Q_OBJECT
  public:
    PartialIndexOperation( const QString& fileName, IndexingData& indexingData, AtomicFlag& interruptRequest )
        : IndexOperation( fileName, indexingData, interruptRequest )
    {
    }

    bool start() override;
};

class LogDataWorker : public QObject {
    Q_OBJECT

  public:
    // Pass a pointer to the IndexingData (initially empty)
    // This object will change it when indexing (IndexingData must be thread safe!)
    explicit LogDataWorker( IndexingData& indexing_data );
    ~LogDataWorker() override;

    // Attaches to a file on disk. Attaching to a non existant file
    // will work, it will just appear as an empty file.
    void attachFile( const QString& fileName );
    // Instructs the thread to start a new full indexing of the file, sending
    // signals as it progresses.
    void indexAll( QTextCodec* forcedEncoding = nullptr );
    // Instructs the thread to start a partial indexing (starting at
    // the end of the file as indexed).
    void indexAdditionalLines();
    // Interrupts the indexing if one is in progress
    void interrupt();

  signals:
    // Sent during the indexing process to signal progress
    // percent being the percentage of completion.
    void indexingProgressed( int percent );
    // Sent when indexing is finished, signals the client
    // to copy the new data back.
    void indexingFinished( LoadingStatus status );

  private slots:
    void onOperationFinished();

  private:
    void doIndexAll();

    bool connectSignalsAndRun( IndexOperation* operationRequested );

    QFuture<bool> operationFuture_;
    QFutureWatcher<bool> operationWatcher_;
    AtomicFlag interruptRequest_;

    // Mutex to protect operationRequested_ and friends
    QMutex mutex_;
    QString fileName_;

    // Pointer to the owner's indexing data (we modify it)
    IndexingData& indexing_data_;
};

#endif
