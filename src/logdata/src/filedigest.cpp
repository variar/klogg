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

#include "filedigest.h"
#include "xxhash.h"

class DigestInternalState {
  public:
    DigestInternalState()
    {
        m_state = XXH64_createState();
        reset();
    }

    ~DigestInternalState()
    {
        XXH64_freeState( m_state );
    }

    void addData( const char* data, size_t length )
    {
        XXH64_update( m_state, reinterpret_cast<const void*>( data ), length );
    }

    void reset()
    {
        XXH64_reset( m_state, 0 );
    }

    uint64_t digest() const
    {
        return XXH64_digest( m_state );
    }

  private:
    XXH64_state_t* m_state;
};

FileDigest::FileDigest()
    : m_state( std::make_unique<DigestInternalState>() )
{
}

FileDigest::~FileDigest() = default;

FileDigest& FileDigest::addData( const char* data, size_t length )
{
    m_state->addData( data, length );
    return *this;
}

FileDigest& FileDigest::addData( const QByteArray& data )
{
    m_state->addData( data.data(), static_cast<size_t>( data.size() ) );
    return *this;
}

uint64_t FileDigest::digest() const
{
    return m_state->digest();
}

void FileDigest::reset()
{
    m_state->reset();
}
