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

#include <QFile>
#include <QThread>

#include "file_write_helper.h"
#include "logger.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <io.h>
#include <windows.h>
#endif // _WIN32

int main( int argc, const char** argv )
{
    //logging::enableLogging();

    if ( argc < 4 ) {
        LOG_ERROR << "Expected 3 arguments";
    }

    LOG_INFO << "Will write to " << argv[ 1 ] << " lines " << argv[ 2 ] << ", flag " << argv[ 3 ];

    QFile file{ argv[ 1 ] };

    file.open( QIODevice::Unbuffered | QIODevice::WriteOnly | QIODevice::Append );

    if ( !file.isOpen() ) {
        return -1;
    }

    const int numberOfLines = atoi( argv[ 2 ] );
    const auto flag = static_cast<WriteFileModification>( atoi( argv[ 3 ] ) );

    if ( flag == WriteFileModification::Truncate ) {
        LOG_INFO << "Truncating file";
        file.resize( 0 );
    }
    else {
        if ( flag == WriteFileModification::StartWithPartialLineEnd ) {
            file.write( partial_line_end );
        }

        char newLine[ 90 ];
        for ( int i = 0; i < numberOfLines; i++ ) {
            snprintf( newLine, 89,
                      "LOGDATA is a part of glogg, we are going to test it thoroughly, this is "
                      "line %06d\n",
                      i );
            file.write( newLine );

            if ( flag == WriteFileModification::DelayClosingFile ) {
                QThread::sleep( 2 );
            }
        }

        if ( flag == WriteFileModification::EndWithPartialLineBegin ) {
            file.write( partial_line_begin );
        }
    }

#ifdef _WIN32
    FlushFileBuffers( reinterpret_cast<HANDLE>( _get_osfhandle( file.handle() ) ) );
#endif // _WIN32

    file.close();

    file.open( QIODevice::Unbuffered | QIODevice::ReadOnly | QIODevice::Append );

    LOG_INFO << "Write to " << argv[ 1 ] << " finished, size " << file.size();

    file.close();

    return 0;
}
