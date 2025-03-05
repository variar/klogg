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

#include "cpu_info.h"
#include <QtGlobal>

#if defined( Q_OS_WIN )

#include <array>
#include <bitset>
#include <vector>

#include <intrin.h>

CpuInstructions supportedCpuInstructions()
{
    CpuInstructions cpuInstructions = CpuInstructions::NONE;

    std::array<int, 4> cpui;

    // Calling __cpuid with 0x0 as the function_id argument
    // gets the number of the highest valid function ID.
    __cpuid( cpui.data(), 0 );

    const auto nIds = cpui[ 0 ];
    std::vector<std::array<int, 4>> data;
    data.reserve( nIds + 1 );

    for ( int i = 0; i <= nIds; ++i ) {
        __cpuidex( cpui.data(), i, 0 );
        data.push_back( cpui );
    }

    // load bitset with flags for function 0x00000001
    if ( nIds >= 1 ) {
        std::bitset<32> f_1_ECX = data[ 1 ][ 2 ];
        std::bitset<32> f_1_EDX = data[ 1 ][ 3 ];

        if ( f_1_EDX[ 26 ] ) {
            cpuInstructions |= CpuInstructions::SSE2;
        }
        if ( f_1_ECX[ 0 ] ) {
            cpuInstructions |= CpuInstructions::SSE3;
        }
        if ( f_1_ECX[ 9 ] ) {
            cpuInstructions |= CpuInstructions::SSSE3;
        }
        if ( f_1_ECX[ 19 ] ) {
            cpuInstructions |= CpuInstructions::SSE41;
        }
        
        if ( f_1_ECX[ 23 ] ) {
            cpuInstructions |= CpuInstructions::POPCNT;
        }

        if ( f_1_ECX[ 28 ] ) {
            cpuInstructions |= CpuInstructions::AVX;
        }
    }

    // load bitset with flags for function 0x00000007
    if ( nIds >= 7 ) {
        std::bitset<32> f_7_EBX = data[ 7 ][ 1 ];

        if ( f_7_EBX[ 5 ] ) {
            cpuInstructions |= CpuInstructions::AVX2;
        }
    }

    return cpuInstructions;
}
#elif defined (Q_OS_LINUX)
CpuInstructions supportedCpuInstructions()
{
    CpuInstructions cpuInstructions = CpuInstructions::NONE;
#if defined(__loongarch__)
#else
    if ( __builtin_cpu_supports( "avx512f" ) ) {
        cpuInstructions |= CpuInstructions::SSE2;
        cpuInstructions |= CpuInstructions::SSE3;
        cpuInstructions |= CpuInstructions::SSSE3;
        cpuInstructions |= CpuInstructions::SSE41;
        cpuInstructions |= CpuInstructions::AVX;
        cpuInstructions |= CpuInstructions::AVX2;
    }
    else if ( __builtin_cpu_supports( "avx2" ) ) {
        cpuInstructions |= CpuInstructions::SSE2;
        cpuInstructions |= CpuInstructions::SSE3;
        cpuInstructions |= CpuInstructions::SSSE3;
        cpuInstructions |= CpuInstructions::SSE41;
        cpuInstructions |= CpuInstructions::AVX;
        cpuInstructions |= CpuInstructions::AVX2;
    }
    else if ( __builtin_cpu_supports( "avx" ) ) {
        cpuInstructions |= CpuInstructions::SSE2;
        cpuInstructions |= CpuInstructions::SSE3;
        cpuInstructions |= CpuInstructions::SSSE3;
        cpuInstructions |= CpuInstructions::SSE41;
        cpuInstructions |= CpuInstructions::AVX;
    }
    else if ( __builtin_cpu_supports( "sse4.1" ) ) {
        cpuInstructions |= CpuInstructions::SSE2;
        cpuInstructions |= CpuInstructions::SSE3;
        cpuInstructions |= CpuInstructions::SSSE3;
        cpuInstructions |= CpuInstructions::SSE41;
    }
    else if ( __builtin_cpu_supports( "ssse3" ) ) {
        cpuInstructions |= CpuInstructions::SSE2;
        cpuInstructions |= CpuInstructions::SSE3;
        cpuInstructions |= CpuInstructions::SSSE3;
    }
    else if ( __builtin_cpu_supports( "sse3" ) ) {
        cpuInstructions |= CpuInstructions::SSE3;
        cpuInstructions |= CpuInstructions::SSE2;
    }
    else if ( __builtin_cpu_supports( "sse2" ) ) {
        cpuInstructions |= CpuInstructions::SSE2;
    }
    if ( __builtin_cpu_supports( "popcnt" ) ) {
        cpuInstructions |= CpuInstructions::POPCNT;
    }
#endif
    return cpuInstructions;
}
#else
CpuInstructions supportedCpuInstructions()
{
    CpuInstructions cpuInstructions = CpuInstructions::NONE;
    cpuInstructions |= CpuInstructions::SSE2;
    cpuInstructions |= CpuInstructions::SSSE3;
    cpuInstructions |= CpuInstructions::SSE41;
    cpuInstructions |= CpuInstructions::POPCNT;
    return cpuInstructions;
}
#endif
