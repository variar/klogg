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
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <limits>
#include <stdexcept>

#include "compressedlinestorage.h"
#include "containers.h"
#include "cpu_info.h"
#include "linetypes.h"
#include "log.h"

#include <streamvbyte.h>
#include <streamvbytedelta.h>

static constexpr size_t SimdIndexBlockSize = 128;

void CompressedLinePositionStorage::move_from( CompressedLinePositionStorage&& orig ) noexcept
{
    blocks_ = std::move( orig.blocks_ );
    packedLinesStorage_ = std::move( orig.packedLinesStorage_ );
    currentLinesBlock_ = std::move( orig.currentLinesBlock_ );
    currentLinesBlockShifted_ = std::move( orig.currentLinesBlockShifted_ );

    nbLines_ = orig.nbLines_;
    lastPos_ = orig.lastPos_;
    canUseSimdSelect_ = orig.canUseSimdSelect_;

    orig.nbLines_ = 0_lcount;
    orig.lastPos_ = 0_offset;
}

CompressedLinePositionStorage::CompressedLinePositionStorage()
{
    auto requiredInstructions = CpuInstructions::SSE41;
    canUseSimdSelect_ = hasRequiredInstructions( supportedCpuInstructions(), requiredInstructions );
}

CompressedLinePositionStorage::CompressedLinePositionStorage(
    CompressedLinePositionStorage&& orig ) noexcept
{
    move_from( std::move( orig ) );
}

CompressedLinePositionStorage&
CompressedLinePositionStorage::operator=( CompressedLinePositionStorage&& orig ) noexcept
{
    move_from( std::move( orig ) );
    return *this;
}

void CompressedLinePositionStorage::append( OffsetInFile pos )
{
    // Lines must be stored in order
    assert( ( pos > lastPos_ ) || ( pos == 0_offset ) );

    currentLinesBlock_.push_back( pos );
    currentLinesBlockShifted_.push_back(
        type_safe::narrow_cast<uint32_t>( pos.get() - currentLinesBlock_.front().get() ) );

    if ( currentLinesBlock_.size() == SimdIndexBlockSize ) {
        compress_current_block();
    }

    lastPos_ = pos;
    ++nbLines_;
}

void CompressedLinePositionStorage::compress_current_block()
{
    BlockMetadata& block = blocks_.emplace_back();
    block.firstLineOffset = currentLinesBlock_.front();

    const size_t packedLinesSize = streamvbyte_max_compressedbytes( SimdIndexBlockSize );
    packedLinesStorage_.resize( packedLinesStorageUsedSize_ + packedLinesSize );
    block.packetStorageOffset = packedLinesStorageUsedSize_;

    const size_t packedBytes
        = streamvbyte_delta_encode( currentLinesBlockShifted_.data(), SimdIndexBlockSize,
                                    packedLinesStorage_.data() + block.packetStorageOffset, 0 );

    packedLinesStorageUsedSize_ += packedBytes;

    currentLinesBlock_.clear();
    currentLinesBlockShifted_.clear();
}

OffsetInFile CompressedLinePositionStorage::at( LineNumber index ) const
{
    if ( index >= nbLines_ ) {
        LOG_ERROR << "Line number not in storage: " << index.get() << ", storage size is "
                  << nbLines_;
        throw std::runtime_error( "Line number not in storage" );
    }

    const size_t blockIndex = index.get() / SimdIndexBlockSize;
    const size_t indexInBlock = index.get() % SimdIndexBlockSize;

    if ( blockIndex == blocks_.size() ) {
        return currentLinesBlock_[ indexInBlock ];
    }

    const BlockMetadata& block = blocks_[ blockIndex ];
    std::array<uint32_t, SimdIndexBlockSize> unpackedBlock;
    streamvbyte_delta_decode( &packedLinesStorage_[ block.packetStorageOffset ],
                              unpackedBlock.data(), SimdIndexBlockSize, 0 );

    return block.firstLineOffset + OffsetInFile( unpackedBlock[ indexInBlock ] );
}

void CompressedLinePositionStorage::append_list( const klogg::vector<OffsetInFile>& positions )
{
    // This is not very clever, but caching should make it
    // reasonably fast.
    for ( auto position : positions )
        append( position );
}

void CompressedLinePositionStorage::uncompress_last_block()
{
    currentLinesBlock_.resize( SimdIndexBlockSize );
    currentLinesBlockShifted_.resize( SimdIndexBlockSize );
    const BlockMetadata& block = blocks_.back();

    streamvbyte_delta_decode( &packedLinesStorage_[ block.packetStorageOffset ],
                              currentLinesBlockShifted_.data(), SimdIndexBlockSize, 0 );

    std::transform( currentLinesBlockShifted_.begin(), currentLinesBlockShifted_.end(),
                    currentLinesBlock_.begin(), [ &block ]( uint32_t pos ) {
                        return OffsetInFile( pos ) + block.firstLineOffset;
                    } );

    blocks_.pop_back();
}

void CompressedLinePositionStorage::pop_back()
{
    if ( currentLinesBlock_.empty() && !blocks_.empty() ) {
        // Last entry caused block compression, so we need to uncompress it
        // to de-alloc last entry.
        uncompress_last_block();
    }

    if ( !currentLinesBlock_.empty() ) {
        currentLinesBlock_.pop_back();
        currentLinesBlockShifted_.pop_back();
    }

    if ( nbLines_.get() == 0 ) {
        lastPos_ = 0_offset;
    }
    else {
        --nbLines_;
        lastPos_ = nbLines_.get() > 0 ? at( nbLines_.get() - 1 ) : 0_offset;
    }
}

size_t CompressedLinePositionStorage::allocatedSize() const
{
    return packedLinesStorage_.size() + blocks_.size() * sizeof( BlockMetadata );
}

klogg::vector<OffsetInFile> CompressedLinePositionStorage::range( LineNumber firstLine,
                                                                  LinesCount count ) const
{
    const size_t firstBlockIndex = firstLine.get() / SimdIndexBlockSize;
    const size_t indexInFirstBlock = firstLine.get() % SimdIndexBlockSize;

    const LineNumber lastLine = firstLine + count - 1_lcount;
    const size_t lastBlockIndex = lastLine.get() / SimdIndexBlockSize;
    const size_t indexInLastBlock = lastLine.get() % SimdIndexBlockSize;

    klogg::vector<OffsetInFile> result;
    result.reserve( count.get() );

    if ( firstBlockIndex == blocks_.size() ) {
        std::copy( currentLinesBlock_.begin() + static_cast<int64_t>( indexInFirstBlock ),
                   currentLinesBlock_.begin() + static_cast<int64_t>( indexInLastBlock + 1 ),
                   std::back_inserter( result ) );
    }
    else {
        size_t lastBlockToUnpack = std::min( lastBlockIndex, blocks_.size() - 1 );
        for ( size_t blockIndex = firstBlockIndex; blockIndex <= lastBlockToUnpack; ++blockIndex ) {
            const BlockMetadata& block = blocks_[ blockIndex ];
            std::array<uint32_t, SimdIndexBlockSize> unpackedBlock;
            streamvbyte_delta_decode( &packedLinesStorage_[ block.packetStorageOffset ],
                                      unpackedBlock.data(), SimdIndexBlockSize, 0 );
            const size_t copyFromIndex = blockIndex == firstBlockIndex ? indexInFirstBlock : 0u;
            const size_t copyToIndex
                = blockIndex == lastBlockIndex ? indexInLastBlock + 1 : unpackedBlock.size();

            std::transform( unpackedBlock.begin() + copyFromIndex,
                            unpackedBlock.begin() + copyToIndex, std::back_inserter( result ),
                            [ &block ]( uint32_t pos ) {
                                return OffsetInFile( pos ) + block.firstLineOffset;
                            } );
        }

        if ( lastBlockIndex == blocks_.size() ) {
            std::copy( currentLinesBlock_.begin(),
                       currentLinesBlock_.begin() + static_cast<int64_t>( indexInLastBlock + 1 ),
                       std::back_inserter( result ) );
        }
    }

    return result;
}
