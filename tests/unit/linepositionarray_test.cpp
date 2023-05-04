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

#include <catch2/catch.hpp>

#include "log.h"

#include "linepositionarray.h"

#include <algorithm>
#include <random>
#include <vector>

#include <configuration.h>

SCENARIO( "LinePositionArray with small number of lines", "[linepositionarray]" )
{

    std::array<LineOffset, 6> offsets = { 4_offset,     8_offset, 10_offset,
                                          345_offset,   // A longer (>128) line
                                          20000_offset, // An even longer (>16384) line
                                          20020_offset };

    GIVEN( "LinePositionArray with small number of lines" )
    {

        LinePositionArray line_array;

        for ( const auto& offset : offsets ) {
            line_array.append( offset );
        }

        REQUIRE( line_array.size() == 6_lcount );

        WHEN( "Access items in linear order" )
        {
            THEN( "Corrent offsets returned" )
            {
                for ( auto i = 0u; i < offsets.size(); ++i ) {
                    REQUIRE( line_array.at( i ) == offsets[ i ] );
                }
            }
        }

        WHEN( "Access items in random order" )
        {
            std::random_device rd;
            std::mt19937 g( rd() );

            auto index = std::vector<uint32_t>( offsets.size() );
            std::generate( index.begin(), index.end(), [ n = 0u ]() mutable { return n++; } );

            THEN( "Corrent offsets returned" )
            {
                for ( auto i : index ) {
                    std::cout << "Test " << i << std::endl;
                    REQUIRE( line_array.at( i ) == offsets[ i ] );
                }
            }
        }

        WHEN( "Adding fake lf" )
        {
            line_array.setFakeFinalLF();

            THEN( "Last offset is returned" )
            {
                REQUIRE( line_array.at( 5 ) == offsets[ 5 ] );
            }
        }

        WHEN( "Adding line after fake lf" )
        {
            line_array.setFakeFinalLF();
            line_array.append( 20030_offset );

            THEN( "New last offset is returned" )
            {
                REQUIRE( line_array.at( 5 ) == 20030_offset );
            }
        }

        WHEN( "Add line to single line array with fake lf" )
        {
            LinePositionArray one_line_array;
            one_line_array.append(10_offset);
            one_line_array.setFakeFinalLF();
            one_line_array.append(20_offset);
            THEN( "New last offset is returned" )
            {
                REQUIRE( one_line_array.at( 0 ) == 20_offset );
            }
        }

        WHEN( "Appending other array " )
        {

            GIVEN( "FastLinePositionArray" )
            {
                FastLinePositionArray other_array;
                other_array.append( 150000_offset );
                other_array.append( 150023_offset );

                WHEN( "Appending other array without fake lf" )
                {
                    line_array.append_list( other_array );

                    THEN( "All lines are kept" )
                    {
                        for ( auto i = 0u; i < offsets.size(); ++i ) {
                            REQUIRE( line_array.at( i ) == offsets[ i ] );
                        }
                        REQUIRE( line_array.at( 6 ) == other_array.at( 0 ));
                        REQUIRE( line_array.at( 7 ) == other_array.at( 1 ));
                    }
                }

                WHEN( "Appending after fake lf" )
                {
                    line_array.setFakeFinalLF();
                    line_array.append_list( other_array );

                    THEN( "Last line is popped back" )
                    {
                        REQUIRE( line_array.size() == 7_lcount );
                        for ( auto i = 0u; i < offsets.size() - 1; ++i ) {
                            REQUIRE( line_array.at( i ) == offsets[ i ] );
                        }
                        REQUIRE( line_array.at( 5 ) == other_array.at( 0 ));
                        REQUIRE( line_array.at( 6 ) == other_array.at( 1 ));
                    }
                }
            }
        }
    }
}

SCENARIO( "LinePositionArray with full block of lines", "[linepositionarray]" )
{

    GIVEN( "LinePositionArray with block of lines" )
    {

        LinePositionArray line_array;

        // Add 255 lines (of various sizes)
        const int lines = 255;
        for ( int i = 0; i < lines; ++i )
            line_array.append( LineOffset( i * 4 ) );
        // Line no 256
        line_array.append( LineOffset( 255 * 4 ) );

        WHEN( "Adding line after block" )
        {
            // Add line no 257
            line_array.append( LineOffset( 255 * 4 + 10 ) );

            THEN( "Correct offset is returned" )
            {
                REQUIRE( line_array.at( 256 ) == LineOffset( 255 * 4 + 10 ) );
            }
        }

        WHEN( "Adding lines after fake lf" )
        {
            THEN( "Correct offset is returned" )
            for ( uint32_t i = 0; i < 1000; ++i ) {
                int64_t pos = ( 257LL * 4 ) + i * 35LL;
                line_array.append( LineOffset( pos ) );
                line_array.setFakeFinalLF();
                REQUIRE( line_array.at( 256 + i ) == LineOffset( pos ) );
                line_array.append( LineOffset( pos + 21LL ) );
                REQUIRE( line_array.at( 256 + i ) == LineOffset( pos + 21LL ) );
            }
        }
    }
}

SCENARIO( "LinePositionArray with UINT32_MAX offsets", "[linepositionarray]" )
{

    std::array<LineOffset, 9> offsets = { 4_offset,
                                          8_offset,
                                          LineOffset( UINT32_MAX - 10 ),
                                          LineOffset( (uint64_t)UINT32_MAX + 10LL ),
                                          LineOffset( (uint64_t)UINT32_MAX + 30LL ),
                                          LineOffset( (uint64_t)2 * UINT32_MAX ),
                                          LineOffset( (uint64_t)2 * UINT32_MAX + 10LL ),
                                          LineOffset( (uint64_t)2 * UINT32_MAX + 1000LL ),
                                          LineOffset( (uint64_t)3 * UINT32_MAX ) };

    GIVEN( "LinePositionArray with long offsets" )
    {
        LinePositionArray line_array;

        for ( const auto& offset : offsets ) {
            line_array.append( offset );
        }

        REQUIRE( line_array.size() == 9_lcount );

        WHEN( "Access items in linear order" )
        {
            THEN( "Corrent offsets returned" )
            {
                for ( auto i = 0u; i < offsets.size(); ++i ) {
                    REQUIRE( line_array.at( i ) == offsets[ i ] );
                }
            }
        }

        WHEN( "Adding lines after fake lf" )
        {
            THEN( "Correct offset is returned" )
            for ( uint32_t i = 0; i < 1000; ++i ) {
                int64_t pos = 3LL * UINT32_MAX + 524LL + i * 35LL;
                line_array.append( LineOffset( pos ) );
                line_array.setFakeFinalLF();
                REQUIRE( line_array.at( 9 + i ) == LineOffset( pos ) );
                line_array.append( LineOffset( pos + 21LL ) );
                REQUIRE( line_array.at( 9 + i ) == LineOffset( pos + 21LL ) );
            }
        }
    }

    GIVEN( "LinePositionArray with small lines" )
    {

        LinePositionArray line_array;
        line_array.append( 4_offset );
        line_array.append( 8_offset );

        WHEN( "Appending large lines" )
        {
            FastLinePositionArray other_array;
            other_array.append( LineOffset( (uint64_t)UINT32_MAX + 10LL ) );
            other_array.append( LineOffset( (uint64_t)UINT32_MAX + 30LL ) );

            line_array.append_list( other_array );

            THEN( "Correct offsets are returned" )
            {
                REQUIRE( line_array.size() == 4_lcount );

                REQUIRE( line_array.at( 0 ) == 4_offset );
                REQUIRE( line_array.at( 1 ) == 8_offset );
                REQUIRE( line_array.at( 2 ) == LineOffset( (uint64_t)UINT32_MAX + 10 ) );
                REQUIRE( line_array.at( 3 ) == LineOffset( (uint64_t)UINT32_MAX + 30 ) );
            }
        }
    }
}
