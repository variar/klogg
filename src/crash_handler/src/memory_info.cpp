/*
 * Copyright (C) 2020 Anton Filimonov and other contributors
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

#include <mutex>

#include <QtGlobal>

#include "memory_info.h"

#if defined( Q_OS_WIN )

#include <windows.h>

#include <psapi.h>

uint64_t systemPhysicalMemory()
{
    MEMORYSTATUSEX status;
    status.dwLength = sizeof( status );
    GlobalMemoryStatusEx( &status );
    return static_cast<uint64_t>( status.ullTotalPhys );
}

uint64_t usedMemory()
{
    PROCESS_MEMORY_COUNTERS_EX pmc;
    GetProcessMemoryInfo( GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof( pmc ) );
    return static_cast<uint64_t>( pmc.PagefileUsage );
}

#elif defined( Q_OS_APPLE )

#include <sys/sysctl.h>
#include <sys/types.h>

#include <AvailabilityMacros.h>
#include <mach/mach.h>
#include <mach/shared_region.h>
#include <unistd.h>

#if SHARED_TEXT_REGION_SIZE || SHARED_DATA_REGION_SIZE
static constexpr size_t SharedSize = SHARED_TEXT_REGION_SIZE + SHARED_DATA_REGION_SIZE;
#else
static constexpr size_t SharedSize = 0;
#endif

uint64_t systemPhysicalMemory()
{
    uint64_t memory;
    size_t size = sizeof( memory );
    sysctlbyname( "hw.memsize", &memory, &size, NULL, 0 );
    return memory;
}

uint64_t usedMemory()
{
    kern_return_t status;
    task_basic_info info;
    mach_msg_type_number_t msgType = TASK_BASIC_INFO_COUNT;
    status = task_info( mach_task_self(), TASK_BASIC_INFO, reinterpret_cast<task_info_t>( &info ),
                        &msgType );
    if ( status == KERN_SUCCESS ) {
        return info.virtual_size - SharedSize;
    }
    else {
        return 0;
    }
}

#else

#include <unistd.h>

#include <array>
#include <cstdio>

uint64_t systemPhysicalMemory()
{
    const auto pageSize = static_cast<uint64_t>( sysconf( _SC_PAGE_SIZE ) );
    const auto pages = static_cast<uint64_t>( sysconf( _SC_PHYS_PAGES ) );
    return pages * pageSize;
}

uint64_t usedMemory()
{
    long unsigned size = 0;
    FILE* statusFile = fopen( "/proc/self/status", "r" );
    if ( statusFile == NULL ) {
        return 0;
    }

    std::array<char, 200> status;
    static constexpr auto pattern = "VmSize: %lu";
    while ( NULL != fgets( status.data(), status.size(), statusFile ) ) {
        if ( 1 == sscanf( status.data(), pattern, &size ) ) {
            break;
        }
    }

    fclose( statusFile );
    return size * 1024;
}

#endif

namespace {
std::once_flag totalMemoryFlag;
static uint64_t totalMemory{};
} // namespace

uint64_t physicalMemory()
{
    std::call_once( totalMemoryFlag, []() { totalMemory = systemPhysicalMemory(); } );
    return totalMemory;
}