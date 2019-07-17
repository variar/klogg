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

#include <catch.hpp>

#include <iostream>

#include <QTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QProcess>
#include <QThread>

#include "log.h"
#include "test_utils.h"
#include "file_write_helper.h"

#include "data/logdata.h"


static const qint64 SL_NB_LINES = 500LL;
static const qint64 VBL_NB_LINES = 50000LL;

namespace {

    class WriteFileThread : public QThread
    {
        Q_OBJECT
    public:
        WriteFileThread(QFile* file, int numberOfLines = 200, WriteFileModification flag = WriteFileModification::None)
            : file_{ file }, numberOfLines_{numberOfLines}, flag_{flag}
        {}

    protected:
        void run() override
        {
            QString writeHelper = QCoreApplication::applicationDirPath() + QDir::separator() + QLatin1Literal("file_write_helper");
            QStringList arguments;
            arguments << file_->fileName() << QString::number(numberOfLines_) << QString::number(static_cast<uint8_t>(flag_));

            const auto result = QProcess::execute(writeHelper, arguments);
            LOG(logINFO) << "Write helper result " << result;
        }

    private:
        QFile* file_;
        int numberOfLines_;
        WriteFileModification flag_;

    };

#include "logdataTest.moc"

#ifdef _WIN32
    void writeDataToFileBackground(QFile& file, int numberOfLines = 200, WriteFileModification flag = WriteFileModification::None) {
        auto thread = new WriteFileThread(&file, numberOfLines, flag);
        thread->start();
        QObject::connect(thread, &WriteFileThread::finished, thread, &WriteFileThread::deleteLater);
    }
#endif
    void writeDataToFile(QFile& file, int numberOfLines = 200, WriteFileModification flag = WriteFileModification::None) {
        auto thread = new WriteFileThread(&file, numberOfLines, flag);
        thread->start();
        thread->wait();
        thread->deleteLater();
    }
}


TEST_CASE("Logdata reading changing file", "[logdata]") {

    QTemporaryDir tempDir;

    LogData log_data;

    auto finishedSpy = std::make_unique<SafeQSignalSpy>( &log_data, SIGNAL( loadingFinished( LoadingStatus ) ) );
    SafeQSignalSpy progressSpy( &log_data, SIGNAL( loadingProgressed( int ) ) );
    SafeQSignalSpy changedSpy( &log_data,
            SIGNAL( fileChanged( LogData::MonitoredFileStatus ) ) );

    // Generate a small file
    QFile file { tempDir.path() + QDir::separator() + QLatin1Literal("testlog.txt") };
    if ( file.open( QIODevice::ReadWrite | QIODevice::Truncate ) ) {
        writeDataToFile( file );
    }

    // Start loading it
    log_data.attachFile( file.fileName() );

    // and wait for the signal
    REQUIRE( finishedSpy->safeWait() );

    // Check we have the small file
    REQUIRE( finishedSpy->count() == 1 );
    REQUIRE( log_data.getNbLine() == 200_lcount );
    REQUIRE( log_data.getMaxLength() == LineLength( SL_LINE_LENGTH ) );
    REQUIRE( log_data.getFileSize() == 200 * (SL_LINE_LENGTH+1LL) );

    auto finishedSpyCount = finishedSpy->count();
    // Add some data to it
    if ( file.isOpen() ) {
        // To test the edge case when the final line is not complete
#ifdef Q_OS_WIN
        writeDataToFileBackground( file, 200, WriteFileModification::EndWithPartialLineBegin );
#else
        writeDataToFile(file, 200, WriteFileModification::EndWithPartialLineBegin);
#endif
    }

    // and wait for the signals
    finishedSpy->wait( 10000 );

    REQUIRE( finishedSpy->count() > finishedSpyCount );

    // Check we have a bigger file
    REQUIRE( changedSpy.count() >= 1 );
    REQUIRE( finishedSpy->count() == 2 );
    REQUIRE( log_data.getNbLine() == 401_lcount );
    REQUIRE( log_data.getMaxLength() == LineLength( SL_LINE_LENGTH ) );
    REQUIRE( log_data.getFileSize() == (qint64) (400 * (SL_LINE_LENGTH+1LL)
            + strlen( partial_line_begin ) ) );

    {
        finishedSpy = std::make_unique<SafeQSignalSpy>( &log_data, SIGNAL( loadingFinished( LoadingStatus ) ) );

        // Add a couple more lines, including the end of the unfinished one.
        if ( file.isOpen() ) {
#ifdef Q_OS_WIN
            writeDataToFileBackground( file, 20, WriteFileModification::StartWithPartialLineEnd );
#else
            writeDataToFile(file, 20, WriteFileModification::StartWithPartialLineEnd);
#endif
        }

        // and wait for the signals
        REQUIRE( finishedSpy->wait( 10000 ) );

        // Check we have a bigger file
        REQUIRE( changedSpy.count() >= 2 );
        REQUIRE( finishedSpy->count() == 1 );
        REQUIRE( log_data.getNbLine() == 421_lcount );
        REQUIRE( log_data.getMaxLength() == LineLength( SL_LINE_LENGTH ) );
        REQUIRE( log_data.getFileSize() == (qint64) ( 420 * (SL_LINE_LENGTH+1LL)
                + strlen( partial_line_begin ) + strlen( partial_line_end ) ) );
    }

    {
        finishedSpy = std::make_unique<SafeQSignalSpy>( &log_data, SIGNAL( loadingFinished( LoadingStatus ) ) );

        // Truncate the file
        QVERIFY( file.resize( 0 ) );

        // and wait for the signals
        REQUIRE( finishedSpy->safeWait() );

        // Check we have an empty file
        REQUIRE( changedSpy.count() >= 3 );
        REQUIRE( finishedSpy->count() == 1 );
        REQUIRE( log_data.getNbLine() == 0_lcount );
        REQUIRE( log_data.getMaxLength().get() == 0 );
        REQUIRE( log_data.getFileSize() == 0LL );
    }
}

SCENARIO( "Attaching log data to files", "[logdata]" ) {

    GIVEN( "Small and big files" ) {
        QTemporaryFile smallFile;
        QTemporaryFile bigFile;

        if ( smallFile.open() ) {
           writeDataToFile( smallFile, SL_NB_LINES );
        }

        if ( bigFile.open() ) {
            writeDataToFile( bigFile, VBL_NB_LINES );
        }

        WHEN( "Interrupt loading" ) {
            LogData log_data;
            SafeQSignalSpy endSpy( &log_data, SIGNAL( loadingFinished( LoadingStatus ) ) );

            // Start loading the VBL
            log_data.attachFile( bigFile.fileName() );

            // Immediately interrupt the loading
            log_data.interruptLoading();

            REQUIRE( endSpy.safeWait( 10000 ) );

            THEN( "No file is attached" ) {
                // Check we have an empty file
                REQUIRE( endSpy.count() == 1 );
                QList<QVariant> arguments = endSpy.takeFirst();
                REQUIRE( arguments.at(0).toInt() ==
                        static_cast<int>( LoadingStatus::Interrupted ) );

                REQUIRE( log_data.getNbLine() == 0_lcount );
                REQUIRE( log_data.getMaxLength().get() == 0 );
                REQUIRE( log_data.getFileSize() == 0LL );
            }
        }

        WHEN( "Try to reattach" ) {
            LogData log_data;
            SafeQSignalSpy endSpy( &log_data, SIGNAL( loadingFinished( LoadingStatus ) ) );

            log_data.attachFile( smallFile.fileName() );
            endSpy.safeWait( 10000 );

            THEN( "Throws" ) {
                CHECK_THROWS_AS( log_data.attachFile( bigFile.fileName() ), CantReattachErr);
            }
        }
    }
}

