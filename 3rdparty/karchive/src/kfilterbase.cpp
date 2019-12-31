/* This file is part of the KDE libraries
   Copyright (C) 2000-2005 David Faure <faure@kde.org>

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

#include "kfilterbase.h"

#include <QtCore/QIODevice>

class KFilterBasePrivate
{
public:
    KFilterBasePrivate()
        : m_flags(KFilterBase::WithHeaders)
        ,  m_dev(nullptr)
        , m_bAutoDel(false)
    {}
    KFilterBase::FilterFlags m_flags;
    QIODevice *m_dev;
    bool m_bAutoDel;
};

KFilterBase::KFilterBase()
    : d(new KFilterBasePrivate)
{
}

KFilterBase::~KFilterBase()
{
    if (d->m_bAutoDel) {
        delete d->m_dev;
    }
    delete d;
}

void KFilterBase::setDevice(QIODevice *dev, bool autodelete)
{
    d->m_dev = dev;
    d->m_bAutoDel = autodelete;
}

QIODevice *KFilterBase::device()
{
    return d->m_dev;
}

bool KFilterBase::inBufferEmpty() const
{
    return inBufferAvailable() == 0;
}

bool KFilterBase::outBufferFull() const
{
    return outBufferAvailable() == 0;
}

bool KFilterBase::terminate()
{
    return true;
}

void KFilterBase::reset()
{
}

void KFilterBase::setFilterFlags(FilterFlags flags)
{
    d->m_flags = flags;
}

KFilterBase::FilterFlags KFilterBase::filterFlags() const
{
    return d->m_flags;
}

void KFilterBase::virtual_hook(int, void *)
{
    /*BASE::virtual_hook( id, data );*/
}
