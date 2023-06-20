/*
 * Copyright (C) 2021 Anton Filimonov and other contributors
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

#include <mimalloc.h>

#include "configuration.h"
#include "logdata.h"
#include "logfiltereddata.h"
#include "dispatch_to.h"
#include "logger.h"
#include "persistentinfo.h"

#include "cli.h"

const bool PersistentInfo::ForcePortable = true;

int main( int argc, char* argv[] )
{
#ifdef KLOGG_USE_MIMALLOC
    mi_stats_reset();
#endif
    qRegisterMetaType<LinesCount>( "LinesCount" );
    qRegisterMetaType<LineNumber>( "LineNumber" );

    QCoreApplication app( argc, argv );
    CliParameters parameters( app, true );

    logging::enableLogging( true, static_cast<logging::LogLevel>( parameters.log_level ) );

    auto configuration = Configuration::getSynced();

    LogData logData;
    auto filteredData = logData.getNewFilteredData();

    filteredData->connect(
        filteredData.get(), &LogFilteredData::searchProgressed,
        [ & ]( LinesCount nbMatches, int progress, LineNumber ) {
            if ( progress == 100 ) {

                LOG_INFO << "Searched finished, got " << nbMatches.get() << " matches";

                const auto defaultChunkSize = 1000_lcount;
                for ( auto chunkStart = 0_lnum; chunkStart < nbMatches;
                      chunkStart = chunkStart + defaultChunkSize ) {
                    auto chunkSize
                        = std::min( defaultChunkSize.get(), nbMatches.get() - chunkStart.get() );
                    auto lines = filteredData->getLines( chunkStart, LinesCount( chunkSize ) );
                    for ( const auto& l : lines ) {
                        std::cout << l.toStdString() << "\n";
                    }
                }

                exit( EXIT_SUCCESS );
            }
        } );

    logData.connect( &logData, &LogData::loadingFinished, [ & ]() {
        dispatchToMainThread(
            [ & ] { filteredData->runSearch( RegularExpressionPattern( parameters.pattern ) ); } );
    } );

    logData.attachFile( parameters.filenames.front() );
    return app.exec();
}