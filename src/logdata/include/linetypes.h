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

#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>

#include <named_type/named_type.hpp>

#include <QMetaType>
#include <QString>

#include "log.h"

using LineOffset
    = fluent::NamedType<int64_t, struct line_offset, fluent::Addable, fluent::Incrementable,
                        fluent::Subtractable, fluent::Comparable, fluent::Printable>;

using LineNumber = fluent::NamedType<uint64_t, struct line_number, fluent::Incrementable,
                                     fluent::Decrementable, fluent::Comparable, fluent::Printable>;

using LinesCount = fluent::NamedType<uint64_t, struct lines_count, fluent::Addable,
                                     fluent::Incrementable, fluent::Subtractable,
                                     fluent::Decrementable, fluent::Comparable, fluent::Printable>;

using LineLength
    = fluent::NamedType<qsizetype, struct line_length, fluent::Comparable, fluent::Printable>;

inline constexpr LineOffset operator"" _offset( unsigned long long int value )
{
    return LineOffset( static_cast<LineOffset::UnderlyingType>( value ) );
}
inline constexpr LineNumber operator"" _lnum( unsigned long long int value )
{
    return LineNumber( static_cast<LineNumber::UnderlyingType>( value ) );
}
inline constexpr LinesCount operator"" _lcount( unsigned long long int value )
{
    return LinesCount( static_cast<LinesCount::UnderlyingType>( value ) );
}
inline constexpr LineLength operator"" _length( unsigned long long int value )
{
    return LineLength( static_cast<LineLength::UnderlyingType>( value ) );
}

template <typename StrongType>
constexpr StrongType maxValue()
{
    return StrongType( std::numeric_limits<typename StrongType::UnderlyingType>::max() );
}

using OptionalLineNumber = std::optional<LineNumber>;

template <typename T, typename Parameter, template <typename> class... Skills>
QDebug operator<<( QDebug dbg, fluent::NamedType<T, Parameter, Skills...> const& object )
{
    QDebugStateSaver saver( dbg );
    dbg << object.get();

    return dbg;
}

// Represents a position in a file (line, column)
class FilePosition {
  public:
    FilePosition()
        : column_{ -1 }
    {
    }
    FilePosition( LineNumber line, int column )
        : line_{ line }
        , column_{ column }
    {
    }

    LineNumber line() const
    {
        return line_;
    }
    int column() const
    {
        return column_;
    }

  private:
    LineNumber line_;
    int column_;
};

inline LineNumber operator+( const LineNumber& number, const LinesCount& count )
{
    return ( number.get() <= maxValue<LineNumber>().get() - count.get() )
               ? LineNumber( number.get() + count.get() )
               : maxValue<LineNumber>();
}

inline LineNumber operator-( const LineNumber& number, const LinesCount& count )
{
    return number.get() >= count.get() ? LineNumber( number.get() - count.get() )
                                       : LineNumber( 0u );
}

inline LinesCount operator-( const LineNumber& n1, const LineNumber& n2 )
{
    return n1.get() >= n2.get() ? LinesCount( n1.get() - n2.get() ) : LinesCount( 0u );
}

inline bool operator<( const LineNumber& number, const LinesCount& count )
{
    return number.get() < count.get();
}
inline bool operator>( const LineNumber& number, const LinesCount& count )
{
    return count.get() < number.get();
}
inline bool operator<=( const LineNumber& number, const LinesCount& count )
{
    return !( count.get() < number.get() );
}
inline bool operator>=( const LineNumber& number, const LinesCount& count )
{
    return !( number < count );
}
inline bool operator==( const LineNumber& number, const LinesCount& count )
{
    return !( number < count ) && !( count.get() < number.get() );
}
inline bool operator!=( const LineNumber& number, const LinesCount& count )
{
    return !( number == count );
}

Q_DECLARE_METATYPE( LineLength )
Q_DECLARE_METATYPE( LineNumber )
Q_DECLARE_METATYPE( LinesCount )

// Length of a tab stop
constexpr qsizetype TabStop = 8;

inline QString untabify( QString&& line, qsizetype initialPosition = 0 )
{
    qsizetype totalSpaces = 0;
    line.replace( QChar::Null, QChar::Space );

    qsizetype position = 0;
    position = line.indexOf( QChar::Tabulation, position );
    while ( position >= 0 ) {
        const auto spaces = TabStop - ( ( initialPosition + position + totalSpaces ) % TabStop );
        line.replace( position, 1, QString( spaces, QChar::Space ) );
        totalSpaces += spaces - 1;
        position = line.indexOf( QChar::Tabulation, position );
    }

    return std::move( line );
}

template <typename LineType>
LineLength getUntabifiedLength( const LineType& utf8Line )
{
    size_t totalSpaces = 0;

    auto tabPosition = utf8Line.find( '\t' );
    while ( tabPosition != std::string_view::npos ) {
        const auto spaces = TabStop - ( ( tabPosition + totalSpaces ) % TabStop ) - 1;
        totalSpaces += spaces;

        tabPosition = utf8Line.find( '\t', tabPosition + 1 );
    }

    return LineLength( static_cast<int>( utf8Line.size() + totalSpaces ) );
}
