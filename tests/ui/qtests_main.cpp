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

#include <catch2/catch_session.hpp>

#include <QApplication>
#include <QMetaType>
#include <QtConcurrent>

#include <configuration.h>
#include <linetypes.h>
#include <highlighterset.h>
#include <persistentinfo.h>

#include <logger.h>

const bool PersistentInfo::ForcePortable = true;

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

  public Q_SLOTS:
    void process()
    {
        result_ = Catch::Session().run( argc_, argv_ );
        Q_EMIT finished( result_ );
    }

  Q_SIGNALS:
    void finished( int );

  private:
    int argc_;
    char** argv_;

    int result_;
};

#include "qtests_main.moc"

int main( int argc, char* argv[] )
{
    QApplication a( argc, argv );

    logging::enableLogging();

    qRegisterMetaType<LinesCount>( "LinesCount" );
    qRegisterMetaType<LineNumber>( "LineNumber" );
    qRegisterMetaType<LineLength>( "LineLength" );

    auto& config = Configuration::getSynced();
    config.setSearchReadBufferSizeLines( 10 );
    config.setIndexReadBufferSizeMb( 1 );
    config.setUseSearchResultsCache( false );

    auto higthlighters = HighlighterSetCollection::getSynced();

#if defined( Q_OS_WIN ) || defined( Q_OS_MAC )
    config.setPollingEnabled( true );
    config.setPollIntervalMs( 1000 );
#else
    config.setPollingEnabled( false );
#endif

    config.setNativeFileWatchEnabled( true );

    QThreadPool::globalInstance()->reserveThread();

    TestRunner* runner = new TestRunner( argc, argv );

    runner->process();
    return runner->result();
}
