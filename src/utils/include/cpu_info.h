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

#ifndef KLOGG_CPU_INFO_H
#define KLOGG_CPU_INFO_H

enum class CpuInstructions : unsigned {
    /// Indicates that no SIMD instructions are supported
    NONE = 0,
    SSE2 = 1 << 1,
    SSE3 = 1 << 2,
    SSSE3 = 1 << 3,
    SSE4_1 = 1 << 4,
    POPCNT = 1 << 5,
    AVX = 1 << 6,
    AVX2 = 1 << 7,
};

inline CpuInstructions& operator|=( CpuInstructions& x, const CpuInstructions& y )
{
    x = static_cast<CpuInstructions>( static_cast<unsigned>( x ) | static_cast<unsigned>( y ) );
    return x;
}

inline CpuInstructions operator&( const CpuInstructions& x, const CpuInstructions& y )
{
    return static_cast<CpuInstructions>( static_cast<unsigned>( x ) & static_cast<unsigned>( y ) );
}

inline CpuInstructions operator~( const CpuInstructions& x )
{
    return static_cast<CpuInstructions>( ~static_cast<unsigned>( x ) );
}

inline bool hasRequiredInstructions( CpuInstructions current, CpuInstructions required )
{
    if ( ( ~current & required ) == CpuInstructions::NONE ) {
        return true;
    }
    return false;
}

CpuInstructions supportedCpuInstructions();

#endif