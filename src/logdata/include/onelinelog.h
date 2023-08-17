//
// Created by marvin on 23-7-26.
//
#pragma once

#include <memory>

#include <QByteArray>
#include <QRegularExpression>
#include <QString>

#include "encodingdetector.h"

class OneLineLog {
  public:
    using Length = QByteArray::size_type;

  public:
    OneLineLog() = default;
    OneLineLog( const char* data, Length len, std::shared_ptr<TextDecoder> dec,
                std::shared_ptr<QRegularExpression> reg );

    ~OneLineLog() = default;

    // no copying allowed
    OneLineLog( const OneLineLog& ) = delete;
    OneLineLog& operator=( const OneLineLog& ) = delete;

    OneLineLog& operator=( OneLineLog&& ) = default;
    OneLineLog( OneLineLog&& ) = default;

    QString string() const;

    QString expandedString() const;

    QString process( std::function<void( QString& )> fn ) const;

    inline bool empty() const
    {
        return buffer_.isEmpty() || !decoder_ || !reg_;
    }

  private:
    QByteArray buffer_;
    std::shared_ptr<TextDecoder> decoder_;
    std::shared_ptr<QRegularExpression> reg_;
};
