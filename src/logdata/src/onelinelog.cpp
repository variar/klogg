//
// Created by marvin on 23-7-26.
//

#include "onelinelog.h"

#include <QTextDecoder>

#include "linetypes.h"

OneLineLog::OneLineLog( const char* data, OneLineLog::Length len, std::shared_ptr<TextDecoder> dec,
                        std::shared_ptr<QRegularExpression> reg )
    : buffer_( data, len )
    , decoder_( dec )
    , reg_( reg )
{
}

QString OneLineLog::string() const
{
    if ( empty() ) {
        LOG_WARNING << buffer_.isEmpty() << ",count:" << decoder_.use_count() << ","
                    << reg_.use_count();
        return {};
    }
    auto log = decoder_->decoder->toUnicode( buffer_ );

    if ( reg_->isValid() ) {
        log.remove( *reg_ );
    }

    if ( log.endsWith( QChar::CarriageReturn ) ) {
        log.chop( 1 );
    }
    return log;
}

QString OneLineLog::expandedString() const
{
    if ( empty() ) {
        LOG_INFO << buffer_.isEmpty() << ",count:" << decoder_.use_count() << ","
                 << reg_.use_count();
        return {};
    }
    auto log = decoder_->decoder->toUnicode( buffer_ );

    if ( reg_->isValid() ) {
        log.remove( *reg_ );
    }

    return untabify( std::move( log ) );
}

QString OneLineLog::process( std::function<void( QString& )> fn ) const
{
    if ( empty() ) {
        LOG_INFO << buffer_.isEmpty() << ",count:" << decoder_.use_count() << ","
                 << reg_.use_count();
        return {};
    }

    auto log = decoder_->decoder->toUnicode( buffer_.data(),
                                             type_safe::narrow_cast<int>( buffer_.size() ) );

    fn( log );
    return log;
}
