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

#ifndef BLOCKPOOL_H
#define BLOCKPOOL_H

#include <vector>
#include <cstddef>
#include <cstdint>

class BlockPoolBase
{
public:
    BlockPoolBase( const BlockPoolBase& ) = delete;
    BlockPoolBase& operator =( const BlockPoolBase& ) = delete;

    BlockPoolBase( BlockPoolBase&& other );
    BlockPoolBase& operator=( BlockPoolBase&& other );

    size_t getElementSize() const;
    size_t getPaddedElementSize() const;

    uint8_t* operator[](size_t index);
    const uint8_t* operator[](size_t index) const;

    uint32_t currentBlock() const;

protected:
    BlockPoolBase( size_t elementSize, size_t alignment );

    uint8_t* getBlock( size_t elementsCount );
    uint8_t* resizeLastBlock( size_t newSize );

    void freeLastBlock();

    size_t lastBlockSize() const;

private:
  std::vector<uint8_t> pool_;

  size_t elementSize_;
  size_t alignment_;

  size_t allocationSize_;

  std::vector<size_t> blockIndex_;
};

template<typename ElementType>
class BlockPool : public BlockPoolBase
{
public:
    BlockPool() : BlockPoolBase( sizeof( ElementType ), alignof( ElementType ) )
    {}

    uint32_t get_block( size_t block_elements_count, ElementType initial_position, uint64_t* next_offset )
    {
        auto ptr = getBlock( block_elements_count );
        if ( ptr ) {
            *( reinterpret_cast<ElementType*>( ptr ) ) = initial_position;
            if ( next_offset ) {
                *next_offset = sizeof( ElementType );
            }
        }

        return currentBlock();
    }

    uint8_t* resize_last_block( size_t new_size )
    {
       return resizeLastBlock( new_size );
    }

    uint32_t free_last_block()
    {
        freeLastBlock();
        return currentBlock();
    }
};

#endif // BLOCKPOOL_H
