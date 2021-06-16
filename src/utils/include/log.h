/*
 * Copyright (C) 2016 -- 2019 Anton Filimonov
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

#ifndef KLOGG_LOG_H
#define KLOGG_LOG_H

#include <chrono>

#include <QLatin1String>
#include <QtGlobal>

#include <optional>

#include <plog/Log.h>

namespace plog {
class GloggFormatter {
  public:
    static util::nstring header()
    {
        return util::nstring();
    }

    static util::nstring
    format( const Record& record ) // This method returns a string from a record.
    {
        tm t;
        util::localtime_s( &t, &record.getTime().time );

        util::nostringstream ss;
        ss << std::setfill( PLOG_NSTR( '0' ) ) << std::setw( 2 ) << t.tm_hour << PLOG_NSTR( ":" )
           << std::setfill( PLOG_NSTR( '0' ) ) << std::setw( 2 ) << t.tm_min << PLOG_NSTR( ":" )
           << std::setfill( PLOG_NSTR( '0' ) ) << std::setw( 2 ) << t.tm_sec << PLOG_NSTR( "." )
           << std::setfill( PLOG_NSTR( '0' ) ) << std::setw( 3 ) << record.getTime().millitm
           << PLOG_NSTR( " " );
        ss << std::setfill( PLOG_NSTR( ' ' ) ) << std::setw( 5 ) << std::left
           << severityToString( record.getSeverity() ) << PLOG_NSTR( " " );
        ss << PLOG_NSTR( "[" ) << record.getTid() << PLOG_NSTR( "] " );
        ss << PLOG_NSTR( "[" ) << record.getFunc() << PLOG_NSTR( "@" ) << record.getLine()
           << PLOG_NSTR( "] " );
        ss << record.getMessage() << PLOG_NSTR( "\n" );

        return ss.str();
    }
};

inline void enableLogging( bool isEnabled, uint8_t logLevel )
{
    if ( isEnabled ) {
        const auto severity = static_cast<plog::Severity>( logLevel );
        plog::get<0>()->setMaxSeverity( severity );
        plog::get<1>()->setMaxSeverity( severity );

        LOG_INFO << "Logging enabled at level " << plog::severityToString( severity );
    }
    else {
        LOG_INFO << "Logging disabled";
        plog::get<1>()->setMaxSeverity( plog::none );
    }
}

template <typename T>
Record& operator<<( Record& r, const std::optional<T>& t )
{
    return t ? r << *t : r << "<empty>";
}

inline Record& operator<<( Record& r, const QLatin1String s )
{
    return r << s.data();
}

inline Record& operator<<( Record& r, const QString& data )
{
#ifdef Q_OS_WIN
    return r << data.toStdWString();
#else
    return r << data.toStdString();
#endif
}

inline Record& operator<<( Record& r, const QStringRef& data )
{
    QString qstr;
    return r << qstr.append( data );
}

inline Record& operator<<( Record& r, const std::chrono::microseconds& duration )
{
    return r << static_cast<float>( duration.count() ) / 1000.f << " ms";
}

} // namespace plog
#endif
