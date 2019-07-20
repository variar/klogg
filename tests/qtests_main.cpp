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

#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include <QApplication>
#include <QMetaType>
#include <QtConcurrent>

#include <configuration.h>
#include <data/linetypes.h>
#include <persistentinfo.h>

#include <log.h>
#include <plog/Appenders/ConsoleAppender.h>

const bool PersistentInfo::ConfigFileParameters::forcePortable = true;

class TestRunner : public QObject {
    Q_OBJECT

  public:
    TestRunner( int argc, char** argv )
        : argc_( argc )
        , argv_( argv )
    {
    }

    int result()
    {
        return result_;
    }

  public slots:
    void process()
    {
        result_ = Catch::Session().run( argc_, argv_ );
        emit finished(result_);
    }

  signals:
    void finished(int);

  private:
    int argc_;
    char** argv_;

    int result_;
};

#include "qtests_main.moc"

int main( int argc, char* argv[] )
{
    QApplication a( argc, argv );

    plog::ConsoleAppender<plog::GloggFormatter> appender;
    plog::init( logINFO, &appender );

    qRegisterMetaType<LinesCount>( "LinesCount" );
    qRegisterMetaType<LineNumber>( "LineNumber" );
    qRegisterMetaType<LineLength>( "LineLength" );

    auto& config = Persistable::getSynced<Configuration>();
    config.setSearchReadBufferSizeLines( 10 );
    config.setIndexReadBufferSizeMb( 1 );
    config.setUseSearchResultsCache( false );

#ifdef Q_OS_WIN
    config.setPollingEnabled( true );
    config.setPollIntervalMs( 1000 );
#else
    config.setPollingEnabled( false );
#endif

    QThreadPool::globalInstance()->reserveThread();

    QThread* testThread = new QThread;
    TestRunner* runner = new TestRunner(argc, argv);

    runner->moveToThread(testThread);

    QObject::connect(testThread, &QThread::started, runner, &TestRunner::process);


    QObject::connect(runner, &TestRunner::finished, runner, &TestRunner::deleteLater);

    QObject::connect(runner, &TestRunner::finished, testThread, &QThread::quit);
    QObject::connect(testThread, &QThread::finished, testThread, &QThread::deleteLater);

    QObject::connect(runner, &TestRunner::finished, &a, &QApplication::exit);

    testThread->start();

    return a.exec();
}
