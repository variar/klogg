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

#include <QtEndian>
#include <cassert>
#include <limits>
#include <stdexcept>

#include "compressedlinestorage.h"
#include "linetypes.h"
#include "log.h"

static constexpr size_t IndexBlockSize = 256;

namespace {
// Functions to manipulate blocks

using BlockOffset = CompressedLinePositionStorage::BlockOffset;

void set_value_at_offset( uint8_t* block, const BlockOffset& offset, uint8_t value )
{
    *( block + type_safe::get( offset ) ) = value;
}
template <typename T>
void set_value_at_offset( uint8_t* block, const BlockOffset& offset, T value )
{
    *reinterpret_cast<T*>( block + type_safe::get( offset ) ) = value;
}

template <typename T = uint8_t>
T get_value_at_offset( const uint8_t* block, const BlockOffset& offset )
{
    return *reinterpret_cast<const T*>( block + type_safe::get( offset ) );
}

// Add a one byte relative delta (0-127) and inc pointer
// First bit is always 0
void block_add_one_byte_relative( uint8_t* block, BlockOffset& offset, uint8_t value )
{
    set_value_at_offset( block, offset, value );
    offset += BlockOffset( sizeof( value ) );
}

uint8_t block_get_alignment_offset( uint8_t alignment, uint64_t offset )
{
    return static_cast<uint8_t>( alignment - offset % alignment );
}

// Add a two bytes relative delta (0-16383) and inc pointer
// First 2 bits are always 10
void block_add_two_bytes_relative( uint8_t* block, BlockOffset& offset, uint16_t value )
{
    const uint8_t alignmentOffset
        = block_get_alignment_offset( alignof( uint16_t ), type_safe::get( offset ) );
    if ( alignmentOffset != 0 ) {
        set_value_at_offset( block, offset, 0xC0 | alignmentOffset );
        offset += BlockOffset( alignmentOffset );
    }

    // Stored in big endian format in order to recognise the initial pattern:
    // 10xx xxxx xxxx xxxx
    //  HO byte | LO byte
    set_value_at_offset( block, offset,
                         qToBigEndian( static_cast<uint16_t>( value | ( 1 << 15 ) ) ) );
    offset += BlockOffset( sizeof( value ) );
}

template <typename ElementType>
void block_add_absolute( uint8_t* block, BlockOffset& offset, ElementType value )
{
    uint8_t alignmentOffset
        = block_get_alignment_offset( alignof( uint16_t ), type_safe::get( offset ) );
    if ( alignmentOffset != 0 ) {
        set_value_at_offset( block, offset, 0xC0 | alignmentOffset );
        offset += BlockOffset( alignmentOffset );
    }

    alignmentOffset = block_get_alignment_offset( alignof( ElementType ),
                                                  type_safe::get( offset ) + sizeof( uint16_t ) );

    // 2 bytes marker (actually only the first two bits are tested)
    set_value_at_offset( block, offset, uint8_t( 0xFF ) );
    set_value_at_offset( block + 1, offset, alignmentOffset );
    offset += BlockOffset( sizeof( uint16_t ) + alignmentOffset );

    // Absolute value (machine endian)
    // This might be unaligned, can cause problem on some CPUs
    set_value_at_offset( block, offset, value );
    offset += BlockOffset( sizeof( ElementType ) );
}

// Initialise the passed block for reading, returning
// the initial position and a pointer to the second entry.
template <typename ElementType>
OffsetInFile block_initial_pos( const uint8_t* block, BlockOffset& offset )
{
    offset = BlockOffset( sizeof( ElementType ) );
    return OffsetInFile( *( reinterpret_cast<const ElementType*>( block ) ) );
}

// Give the next position in the block based on the previous
// position, then increase the pointer.
template <typename ElementType>
OffsetInFile block_next_pos( const uint8_t* block, BlockOffset& offset, OffsetInFile previous_pos )
{
    OffsetInFile pos = previous_pos;

    uint8_t byte = get_value_at_offset( block, offset );

    if ( !( byte & 0x80 ) ) {
        // High order bit is 0
        pos += OffsetInFile( byte );
        ++offset;
        return pos;
    }

    if ( byte != 0xFF && ( byte & 0xC0 ) == 0xC0 ) {
        // need to skip aligned bytes;
        const auto alignmentOffset = static_cast<uint8_t>( byte & ( ~0xC0 ) );
        offset += BlockOffset( alignmentOffset );
        byte = get_value_at_offset( block, offset );
    }
    ++offset;

    if ( ( byte & 0xC0 ) == 0x80 ) {
        // Remove the starting 10b
        const auto hi_byte = static_cast<uint16_t>( byte & ( ~0xC0 ) );
        // We need to read the low order byte
        const auto lo_byte = static_cast<uint16_t>( get_value_at_offset( block, offset ) );
        // And form the displacement (stored as big endian)
        pos += OffsetInFile( ( hi_byte << 8 ) | lo_byte );

        ++offset;
    }
    else {
        // skip aligned bytes
        const auto alignmentOffset = get_value_at_offset( block, offset );
        offset += BlockOffset( alignmentOffset + 1u );

        // And read the new absolute pos (machine endian)
        pos = OffsetInFile( get_value_at_offset<ElementType>( block, offset ) );
        offset += BlockOffset( sizeof( ElementType ) );
    }

    return pos;
}
} // namespace

void CompressedLinePositionStorage::move_from( CompressedLinePositionStorage&& orig ) noexcept
{
    nb_lines_ = orig.nb_lines_;
    first_long_line_ = orig.first_long_line_;
    current_pos_ = orig.current_pos_;
    block_index_ = orig.block_index_;
    long_block_index_ = orig.long_block_index_;
    block_offset_ = orig.block_offset_;
    previous_block_offset_ = orig.previous_block_offset_;

    orig.nb_lines_ = 0_lcount;
}

// Move constructor
CompressedLinePositionStorage::CompressedLinePositionStorage(
    CompressedLinePositionStorage&& orig ) noexcept
    : pool32_( std::move( orig.pool32_ ) )
    , pool64_( std::move( orig.pool64_ ) )
{
    move_from( std::move( orig ) );
}

// Move assignement
CompressedLinePositionStorage&
CompressedLinePositionStorage::operator=( CompressedLinePositionStorage&& orig ) noexcept
{
    pool32_ = std::move( orig.pool32_ );
    pool64_ = std::move( orig.pool64_ );
    move_from( std::move( orig ) );
    return *this;
}

void CompressedLinePositionStorage::append( OffsetInFile pos )
{
    // Lines must be stored in order
    assert( ( pos > current_pos_ ) || ( pos == 0_offset ) );

    // Save the pointer in case we need to "pop_back"
    previous_block_offset_ = block_offset_;

    bool store_in_big = false;
    if ( pos.get() > std::numeric_limits<uint32_t>::max() ) {
        store_in_big = true;
        if ( !first_long_line_ ) {
            // First "big" end of line, we will start a new (64) block
            first_long_line_ = LineNumber( nb_lines_.get() );
            block_offset_ = {};
        }
    }

    if ( !block_offset_ ) {
        // We need to start a new block
        size_t nextOffset{};
        if ( !store_in_big ) {
            block_index_ = pool32_.get_block( IndexBlockSize, pos.get<uint32_t>(), &nextOffset );
        }
        else {
            long_block_index_ = pool64_.get_block( IndexBlockSize, pos.get(), &nextOffset );
        }
        block_offset_ = BlockOffset{ nextOffset };
    }
    else {
        const auto block
            = ( !store_in_big ) ? pool32_.at( block_index_ ) : pool64_.at( long_block_index_ );
        auto delta = pos - current_pos_;
        if ( delta < 128_offset ) {
            // Code relative on one byte
            block_add_one_byte_relative( block, block_offset_, delta.get<uint8_t>() );
        }
        else if ( delta < 16384_offset ) {
            // Code relative on two bytes
            block_add_two_bytes_relative( block, block_offset_, delta.get<uint16_t>() );
        }
        else {
            // Code absolute
            if ( !store_in_big )
                block_add_absolute( block, block_offset_, pos.get<uint32_t>() );
            else
                block_add_absolute( block, block_offset_, pos.get() );
        }
    }

    current_pos_ = pos;
    ++nb_lines_;

    const auto shrinkBlock = [ this ]( auto& blockPool ) {
        const auto effective_block_size = type_safe::get( previous_block_offset_ );

        // We allocate extra space for the last element in case it
        // is replaced by an absolute value in the future (following a pop_back)
        const auto new_size = effective_block_size + blockPool.getPaddedElementSize();
        blockPool.resize_last_block( new_size );

        block_offset_ = {};
        previous_block_offset_ = BlockOffset( effective_block_size );
    };

    if ( !store_in_big ) {
        if ( nb_lines_.get() % IndexBlockSize == 0 ) {
            // We have finished the block

            // Let's reduce its size to what is actually used
            shrinkBlock( pool32_ );
        }
    }
    else {
        if ( ( nb_lines_.get() - first_long_line_->get() ) % IndexBlockSize == 0 ) {
            // We have finished the block

            // Let's reduce its size to what is actually used
            shrinkBlock( pool64_ );
        }
    }
}

OffsetInFile CompressedLinePositionStorage::at( LineNumber index, Cache* lastPosition ) const
{
    if ( index >= nb_lines_ ) {
        LOG_ERROR << "Line number not in storage: " << index.get() << ", storage size is "
                  << nb_lines_;
        throw std::runtime_error( "Line number not in storage" );
    }

    auto last_read = lastPosition != nullptr ? *lastPosition : Cache{};

    const uint8_t* block = nullptr;
    BlockOffset offset;
    OffsetInFile position;

    if ( !first_long_line_ || index < *first_long_line_ ) {
        block = pool32_.at( index.get() / IndexBlockSize );

        if ( ( index.get() == last_read.index.get() + 1 )
             && ( index.get() % IndexBlockSize != 0 ) ) {
            position = last_read.position;
            offset = last_read.offset;

            position = block_next_pos<uint32_t>( block, offset, position );
        }
        else {
            position = block_initial_pos<uint32_t>( block, offset );

            for ( uint32_t i = 0; i < index.get() % IndexBlockSize; i++ ) {
                // Go through all the lines in the block till the one we want
                position = block_next_pos<uint32_t>( block, offset, position );
            }
        }
    }
    else {
        const auto index_in_64 = index - *first_long_line_;
        block = pool64_.at( index_in_64.get() / IndexBlockSize );

        if ( ( index.get() == last_read.index.get() + 1 )
             && ( index_in_64.get() % IndexBlockSize != 0 ) ) {
            position = last_read.position;
            offset = last_read.offset;

            position = block_next_pos<OffsetInFile::UnderlyingType>( block, offset, position );
        }
        else {
            position = block_initial_pos<OffsetInFile::UnderlyingType>( block, offset );

            for ( uint32_t i = 0; i < index_in_64.get() % IndexBlockSize; i++ ) {
                // Go through all the lines in the block till the one we want
                position = block_next_pos<OffsetInFile::UnderlyingType>( block, offset, position );
            }
        }
    }

    // Populate our cache ready for next consecutive read
    if ( lastPosition != nullptr ) {
        lastPosition->index = index;
        lastPosition->position = position;
        lastPosition->offset = offset;
    }

    return position;
}

void CompressedLinePositionStorage::append_list( const klogg::vector<OffsetInFile>& positions )
{
    // This is not very clever, but caching should make it
    // reasonably fast.
    for ( auto position : positions )
        append( position );
}

void CompressedLinePositionStorage::pop_back()
{
    // Removing the last entered data, there are two cases
    if ( previous_block_offset_ ) {
        // The last append was a normal entry in an existing block,
        // so we can just revert the pointer
        block_offset_ = previous_block_offset_;
        previous_block_offset_ = {};
    }
    else {
        // A new block has been created for the last entry, we need
        // to de-alloc it.

        if ( !first_long_line_ ) {
            assert( ( nb_lines_.get() - 1 ) % IndexBlockSize == 0 );
            block_index_ = pool32_.free_last_block();
        }
        else {
            assert( ( nb_lines_.get() - first_long_line_->get() - 1 ) % IndexBlockSize == 0 );
            long_block_index_ = pool64_.free_last_block();
        }

        block_offset_ = {};
    }

    if ( nb_lines_.get() == 0 ) {
        current_pos_ = 0_offset;
    }
    else {
        --nb_lines_;
        current_pos_ = nb_lines_.get() > 0 ? at( nb_lines_.get() - 1 ) : 0_offset;
    }
}

size_t CompressedLinePositionStorage::allocatedSize() const
{
    return pool32_.allocatedSize() + pool64_.allocatedSize();
}