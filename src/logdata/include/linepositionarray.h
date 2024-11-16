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

#ifndef LINEPOSITIONARRAY_H
#define LINEPOSITIONARRAY_H

#include <cstddef>
#include <vector>

#include "compressedlinestorage.h"

#include "log.h"

class SimpleLinePositionStorage {
public:
    SimpleLinePositionStorage()
    {
        storage_.reserve( 10000 );
    }

    SimpleLinePositionStorage( const SimpleLinePositionStorage& ) = delete;
    SimpleLinePositionStorage& operator=( const SimpleLinePositionStorage& ) = delete;

    SimpleLinePositionStorage( SimpleLinePositionStorage&& ) = default;
    SimpleLinePositionStorage& operator=( SimpleLinePositionStorage&& ) = default;

    // Append the passed end-of-line to the storage
    void append( OffsetInFile pos )
    {
        storage_.push_back( pos );
    }

    void push_back( OffsetInFile pos )
    {
        append( pos );
    }

    // Size of the array
    LinesCount size() const
    {
        return LinesCount( static_cast<LinesCount::UnderlyingType>( storage_.size() ) );
    }

    size_t allocatedSize() const
    {
        return storage_.capacity();
    }

    // Element at index
    OffsetInFile at( size_t i ) const
    {
        return storage_.at( i );
    }

    OffsetInFile at( LineNumber i ) const
    {
        return at( i.get() );
    }

    // Add one list to the other
    void append_list( const SimpleLinePositionStorage& positions )
    {
        storage_.insert( storage_.end(), positions.storage_.begin(), positions.storage_.end() );
    }

    // Pop the last element of the storage
    void pop_back()
    {
        storage_.pop_back();
    }

    operator const klogg::vector<OffsetInFile>&() const
    {
        return storage_;
    }

private:
    klogg::vector<OffsetInFile> storage_;
};

// This class is a list of end of lines position,
// in addition to a list of uint64_t (positions within the files)
// it can keep track of whether the final LF was added (for non-LF terminated
// files) and remove it when more data are added.
template <typename Storage>
class LinePosition {
public:
    template <typename>
    friend class LinePosition;

    LinePosition() = default;
    LinePosition( const LinePosition& ) = delete;
    LinePosition& operator=( const LinePosition& ) = delete;

    LinePosition( LinePosition&& orig ) noexcept
    {
        *this = std::move( orig );
    }

    LinePosition& operator=( LinePosition&& orig ) noexcept
    {
        array = std::move( orig.array );
        fakeFinalLF_ = orig.fakeFinalLF_;
        return *this;
    }

    // Add a new line position at the given position
    // Invariant: pos must be greater than the previous one
    // (this is NOT checked!)
    inline void append( OffsetInFile pos )
    {
        if ( fakeFinalLF_ )
            array.pop_back();

        array.push_back( pos );
        fakeFinalLF_ = false;
    }
    // Size of the array
    inline LinesCount size() const
    {
        return array.size();
    }

    size_t allocatedSize() const
    {
        return array.allocatedSize();
    }

    // Extract an element
    inline OffsetInFile at( LineNumber::UnderlyingType i ) const
    {
        const auto pos = array.at( i );
        // LOG_DEBUG << "Line pos at " << i << " is " << pos;
        return pos;
    }

    // Set the presence of a fake final LF
    // Must be used after 'append'-ing a fake LF at the end.
    void setFakeFinalLF( bool finalLF = true )
    {
        fakeFinalLF_ = finalLF;
    }

    // Add another list to this one, removing any fake LF on this list.
    // Invariant: all pos in other must be greater than any pos in this
    // (this is NOT checked!)
    void append_list( const LinePosition<SimpleLinePositionStorage>& other )
    {
        // If our final LF is fake, we remove it
        if ( fakeFinalLF_ )
            this->array.pop_back();

        // Append the arrays
        this->array.append_list( other.array );
        // array += other.array;

        // In case the 'other' object has a fake LF
        this->fakeFinalLF_ = other.fakeFinalLF_;
    }

private:
    Storage array;
    bool fakeFinalLF_ = false;
};

// Use the non-optimised storage
using FastLinePositionArray = LinePosition<SimpleLinePositionStorage>;
using LinePositionArray = LinePosition<CompressedLinePositionStorage>;

#endif
