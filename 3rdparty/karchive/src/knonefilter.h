/* This file is part of the KDE libraries
   Copyright (C) 2011 Mario Bensi <mbensi@ipsquad.net>

   Based on kbzip2filter:
   Copyright (C) 2000, 2009 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __knonefilter__h
#define __knonefilter__h

#include "kfilterbase.h"

/**
 * Internal class used by KFilterDev
 *
 * This header is not installed.
 *
 * @internal
 */
class KNoneFilter : public KFilterBase
{
public:
    KNoneFilter();
    virtual ~KNoneFilter();

    bool init(int mode) override;
    int mode() const override;
    bool terminate() override;
    void reset() override;
    bool readHeader() override; // this is about the GZIP header
    bool writeHeader(const QByteArray &fileName) override;
    void setOutBuffer(char *data, uint maxlen) override;
    void setInBuffer(const char *data, uint size) override;
    int  inBufferAvailable() const override;
    int  outBufferAvailable() const override;
    Result uncompress() override;
    Result compress(bool finish) override;

private:
    Result copyData();

    class Private;
    Private *const d;
};

#endif
