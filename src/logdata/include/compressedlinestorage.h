/*
 * Copyright (C) 2015 Nicolas Bonnefon and other contributors
 *
 * This file is part of glogg.
 *
 * glogg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * glogg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with glogg.  If not, see <http://www.gnu.org/licenses/>.
 */

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

#include <cstdint>
#include <vector>

#include "blockpool.h"
#include "linetypes.h"

#include "tbb/enumerable_thread_specific.h"

// This class is a compressed storage backend for LinePositionArray
// It emulates the interface of a vector, but take advantage of the nature
// of the stored data (increasing end of line addresses) to apply some
// compression in memory, while still providing fast, constant-time look-up.

/* The current algorithm takes advantage of the fact most lines are reasonably
 * short, it codes each line on:
 * - Line < 127 bytes    : 1 byte
 * - 127 < line < 16383  : 2 bytes
 * - line > 16383        : 6 bytes (or 10 bytes)
 * Uncompressed backend stores line on 4 bytes or 8 bytes.
 *
 * The algorithm is quite simple, the file is first divided in two parts:
 * - The lines whose end are located before UINT32_MAX
 * - The lines whose end are located after UINT32_MAX
 * Those end of lines are stored separately in the table32 and the table64
 * respectively.
 *
 * The EOL list is then divided in blocks of BLOCK_SIZE (128) lines.
 * A block index vector (per table) contains pointers to each block.
 *
 * Each block is then defined as such:
 * Block32 (sizes in byte)
 * 00 - Absolute EOF address (4 bytes)
 * 04 - ( 0xxx xxxx  if second line is < 127 ) (1 byte, relative)
 *    - ( 10xx xxxx
 *        xxxx xxxx  if second line is < 16383 ) (2 bytes, relative)
 *    - ( 1111 1111
 *        xxxx xxxx
 *        xxxx xxxx  if second line is > 16383 ) (6 bytes, absolute)
 * ...
 * (126 more lines)
 *
 * Block64 (sizes in byte)
 * 00 - Absolute EOF address (8 bytes)
 * 08 - ( 0xxx xxxx  if second line is < 127 ) (1 byte, relative)
 *    - ( 10xx xxxx
 *        xxxx xxxx  if second line is < 16383 ) (2 bytes, relative)
 *    - ( 1111 1111
 *        xxxx xxxx
 *        xxxx xxxx
 *        xxxx xxxx
 *        xxxx xxxx  if second line is > 16383 ) (10 bytes, absolute)
 * ...
 * (126 more lines)
 *
 * Absolute addressing has been adopted for line > 16383 to bound memory usage in case
 * of pathologically long (MBs or GBs) lines, even if it is a bit less efficient for
 * long-ish (30 KB) lines.
 *
 * The table32 always starts at 0, the table64 starts at first_long_line_
 */

#ifndef COMPRESSEDLINESTORAGE_H
#define COMPRESSEDLINESTORAGE_H

// template<int BLOCK_SIZE = 128>
class CompressedLinePositionStorage {
  public:
    // Default constructor
    CompressedLinePositionStorage()
        : block_index_{ 0 }
        , long_block_index_{ 0 }
        , first_long_line_{ std::numeric_limits<LineNumber::UnderlyingType>::max() }
    {
    }

    // Copy constructor would be slow, delete!
    CompressedLinePositionStorage( const CompressedLinePositionStorage& orig ) = delete;
    CompressedLinePositionStorage& operator=( const CompressedLinePositionStorage& orig ) = delete;

    // Move constructor
    CompressedLinePositionStorage( CompressedLinePositionStorage&& orig );
    // Move assignement
    CompressedLinePositionStorage& operator=( CompressedLinePositionStorage&& orig );

    // Append the passed end-of-line to the storage
    void append( LineOffset pos );
    void push_back( LineOffset pos )
    {
        append( pos );
    }

    // Size of the array
    LinesCount size() const
    {
        return nb_lines_;
    }

    // Element at index
    LineOffset at( uint32_t i ) const
    {
        return at( LineNumber( i ) );
    }
    LineOffset at( LineNumber i ) const;

    // Add one list to the other
    void append_list( const std::vector<LineOffset>& positions );

    // Pop the last element of the storage
    void pop_back();

    using BlockOffset = fluent::NamedType<uint64_t, struct block_offset, fluent::Incrementable,
                                          fluent::Addable, fluent::Comparable>;

  private:
    // Utility for move ctor/assign
    void move_from( CompressedLinePositionStorage&& orig );

    // The two indexes
    BlockPool<uint32_t> pool32_;
    BlockPool<uint64_t> pool64_;

    // Total number of lines in storage
    LinesCount nb_lines_;

    // Current position (position of the end of the last line added)
    LineOffset current_pos_;

    uint32_t block_index_;
    uint32_t long_block_index_;

    // The index of the first line whose end is stored in a block64
    // Initialised at UINT32_MAX, meaning "unset"
    // this is the origin point for all calculations in block64
    LineNumber first_long_line_;

    // Offset of the next position (not yet written) within the current
    // block. null means there is no current block (previous block
    // finished or no data)
    BlockOffset block_offset_;

    // For pop_back:

    // Previous offset to block element, it is restored when we
    // "pop_back" the last element.
    // A null here means pop_back need to free the block
    // that has just been created.
    BlockOffset previous_block_offset_;

    // Cache the last position read
    // This is to speed up consecutive reads (whole page)
    struct Cache {
        Cache()
            : index{ std::numeric_limits<uint32_t>::max() - 1U }
            , position{ 0 }
            , offset{ 0 }
        {
        }

        LineNumber index;
        LineOffset position;
        BlockOffset offset;
    };

    mutable tbb::enumerable_thread_specific<Cache> cache_;
};

#endif
