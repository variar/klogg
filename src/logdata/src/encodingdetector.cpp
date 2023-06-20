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

#include "encodingdetector.h"

#include <QTextCodec>

#include "containers.h"
#include "log.h"
#include <uchardet.h>

namespace {

class UchardetHolder {
  public:
    UchardetHolder()
        : ud_{ uchardet_new() }
    {
    }
    ~UchardetHolder()
    {
        uchardet_delete( ud_ );
    }

    UchardetHolder( const UchardetHolder& ) = delete;
    UchardetHolder& operator=( const UchardetHolder& ) = delete;

    UchardetHolder( UchardetHolder&& other ) = delete;
    UchardetHolder& operator=( UchardetHolder&& other ) = delete;

    int handle_data( const char* data, size_t len )
    {
        return uchardet_handle_data( ud_, data, len );
    }

    void data_end()
    {
        uchardet_data_end( ud_ );
    }

    const char* get_charset()
    {
        return uchardet_get_charset( ud_ );
    }

  private:
    uchardet_t ud_;
};

} // namespace

EncodingParameters::EncodingParameters( const QTextCodec* codec )
{
    static constexpr QChar LineFeed( QChar::LineFeed );
    static constexpr int Utf8Mib = 106;
    static constexpr int Utf16LEMib = 1014;
    static constexpr int UsAsciiMib = 3;

    isUtf8Compatible = codec->mibEnum() == Utf8Mib || codec->mibEnum() == UsAsciiMib;
    isUtf16LE = codec->mibEnum() == Utf16LEMib;

    QTextCodec::ConverterState convertState( QTextCodec::IgnoreHeader );
    const QByteArray encodedLineFeed = codec->fromUnicode( &LineFeed, 1, &convertState );

    lineFeedWidth = static_cast<int>( encodedLineFeed.size() );
    lineFeedIndex
        = encodedLineFeed[ 0 ] == '\n' ? 0 : ( static_cast<int>( encodedLineFeed.size() ) - 1 );
}

QTextCodec* EncodingDetector::detectEncoding( const klogg::vector<char>& block ) const
{
    UniqueLock lock( mutex_ );

    UchardetHolder ud;

    auto rc = ud.handle_data( block.data(), block.size() );
    if ( rc == 0 ) {
        ud.data_end();
    }

    QTextCodec* uchardetCodec = nullptr;
    if ( rc == 0 ) {
        auto uchardetGuess = ud.get_charset();
        LOG_DEBUG << "Uchardet encoding guess " << uchardetGuess;
        uchardetCodec = QTextCodec::codecForName( uchardetGuess );
        if ( uchardetCodec ) {
            LOG_DEBUG << "Uchardet codec selected " << uchardetCodec->name().constData();
        }
        else {
            LOG_DEBUG << "Uchardet codec not found for guess " << uchardetGuess;
        }
    }

    QByteArray blockArray = QByteArray::fromRawData( block.data(), klogg::isize( block ) );

    auto encodingGuess = uchardetCodec ? QTextCodec::codecForUtfText( blockArray, uchardetCodec )
                                       : QTextCodec::codecForUtfText( blockArray );

    LOG_DEBUG << "Final encoding guess " << encodingGuess->name().constData();

    return encodingGuess;
}

TextCodecHolder::TextCodecHolder( QTextCodec* codec )
    : codec_{ codec }
    , encodingParams_{ codec }
{
    assert( codec != nullptr );
}

QTextCodec* TextCodecHolder::codec() const
{
    SharedLock guard( mutex_ );
    return codec_;
}

EncodingParameters TextCodecHolder::encodingParameters() const
{
    SharedLock guard( mutex_ );
    return encodingParams_;
}

int TextCodecHolder::mibEnum() const
{
    SharedLock guard( mutex_ );
    return codec_->mibEnum();
}

void TextCodecHolder::setCodec( QTextCodec* codec )
{
    UniqueLock guard( mutex_ );
    codec_ = codec;
    encodingParams_ = EncodingParameters{ codec_ };
}

TextDecoder TextCodecHolder::makeDecoder() const
{
    SharedLock guard( mutex_ );
    return { std::make_unique<QTextDecoder>( codec_ ), encodingParams_ };
}