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

#ifndef ENCODINGDETECTOR_H
#define ENCODINGDETECTOR_H

#include "synchronization.h"

#include <QByteArray>
#include <memory>

class QTextCodec;
class QTextDecoder;

struct EncodingParameters {
    EncodingParameters() = default;
    explicit EncodingParameters( const QTextCodec* codec );

    bool isUtf8Compatible{ false };
    bool isUtf16LE{ false };

    qsizetype lineFeedWidth{ 1 };
    qsizetype lineFeedIndex{ 0 };

    bool operator==( const EncodingParameters& other ) const
    {
        return lineFeedWidth == other.lineFeedWidth && lineFeedIndex == other.lineFeedIndex;
    }

    bool operator!=( const EncodingParameters& other ) const
    {
        return !operator==( other );
    }

    qsizetype getBeforeCrOffset() const
    {
        return lineFeedIndex;
    }

    qsizetype getAfterCrOffset() const
    {
        return lineFeedWidth - lineFeedIndex - 1;
    }
};

class EncodingDetector {
  public:
    static EncodingDetector& getInstance()
    {
        static EncodingDetector instance;
        return instance;
    }

    EncodingDetector( const EncodingDetector& ) = delete;
    EncodingDetector& operator=( const EncodingDetector& ) = delete;
    EncodingDetector( const EncodingDetector&& ) = delete;
    EncodingDetector& operator=( const EncodingDetector&& ) = delete;

    QTextCodec* detectEncoding( const QByteArray& block ) const;

  private:
    EncodingDetector() = default;
    ~EncodingDetector() = default;

    mutable SharedMutex mutex_;
};

struct TextDecoder {
    std::unique_ptr<QTextDecoder> decoder;
    EncodingParameters encodingParams;
};

class TextCodecHolder {
  public:
    explicit TextCodecHolder( QTextCodec* codec );

    void setCodec( QTextCodec* codec );

    QTextCodec* codec() const;
    EncodingParameters encodingParameters() const;
    int mibEnum() const;

    TextDecoder makeDecoder() const;

  private:
    QTextCodec* codec_;
    EncodingParameters encodingParams_;
    mutable SharedMutex mutex_;
};

#endif // ENCODINGDETECTOR_H
