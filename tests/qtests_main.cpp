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
    config.setIndexReadBufferSizeMb(1);
    config.setUseSearchResultsCache( false );


#ifdef Q_OS_WIN
    config.setPollingEnabled( true );
    config.setPollIntervalMs( 1000 );
#else
    config.setPollingEnabled( false );
#endif

    QThreadPool::globalInstance()->reserveThread();
    QtConcurrent::run( [&a, &argc, &argv]() {
        int result = Catch::Session().run( argc, argv );
        a.processEvents();
        a.exit( result );
    } );

    return a.exec();
}
