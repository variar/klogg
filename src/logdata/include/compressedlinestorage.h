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

#include <array>
#include <cstddef>
#include <cstdint>

#include "linetypes.h"
#include <type_safe/strong_typedef.hpp>

// This class is a compressed storage backend for LinePositionArray
// It emulates the interface of a vector, but take advantage of the nature
// of the stored data (increasing end of line addresses) to apply some
// compression in memory, while still providing fast, constant-time look-up.

#ifndef SIMDCOMPRESSEDLINESTORAGE_H
#define SIMDCOMPRESSEDLINESTORAGE_H

class CompressedLinePositionStorage {
public:
    CompressedLinePositionStorage();

    // Copy constructor would be slow, delete!
    CompressedLinePositionStorage( const CompressedLinePositionStorage& orig ) = delete;
    CompressedLinePositionStorage& operator=( const CompressedLinePositionStorage& orig ) = delete;

    CompressedLinePositionStorage( CompressedLinePositionStorage&& orig ) noexcept;
    CompressedLinePositionStorage& operator=( CompressedLinePositionStorage&& orig ) noexcept;

    ~CompressedLinePositionStorage() = default;

    // Append the passed end-of-line to the storage
    void append( OffsetInFile pos );
    void push_back( OffsetInFile pos )
    {
        append( pos );
    }

    // Size of the array
    LinesCount size() const
    {
        return nbLines_;
    }

    size_t allocatedSize() const;

    // Element at index
    OffsetInFile at( size_t i ) const
    {
        return at( LineNumber( i ) );
    }
    OffsetInFile at( LineNumber i ) const;

    // Add one list to the other
    void append_list( const klogg::vector<OffsetInFile>& positions );

    // Pop the last element of the storage
    void pop_back();

private:
    // Utility for move ctor/assign
    void move_from( CompressedLinePositionStorage&& orig ) noexcept;

    void compress_current_block();
    void uncompress_last_block();
    struct BlockMetadata {
        OffsetInFile firstLineOffset{};
        uint8_t packetBitWidth{};
        size_t packetStorageOffset{};
    };

    klogg::vector<BlockMetadata> blocks_;

    struct alignas( 16 ) AlignedStorage {
        std::array<uint8_t, 16> d;
    };
    klogg::vector<AlignedStorage> packedLinesStorage_;

    klogg::vector<OffsetInFile> currentLinesBlock_;
    klogg::vector<uint32_t> currentLinesBlockShifted_;

    // Total number of lines in storage
    LinesCount nbLines_;

    // Current position (position of the end of the last line added)
    OffsetInFile lastPos_;

    bool canUseSimdSelect_{ false };
};

#endif
