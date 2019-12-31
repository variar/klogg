/* This file is part of the KDE libraries
   Copyright (C) 2001, 2002, 2007 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef klimitediodevice_p_h
#define klimitediodevice_p_h

#include <QtCore/QDebug>
#include <QtCore/QIODevice>
/**
 * A readonly device that reads from an underlying device
 * from a given point to another (e.g. to give access to a single
 * file inside an archive).
 * @author David Faure <faure@kde.org>
 * @internal - used by KArchive
 */
class KLimitedIODevice : public QIODevice
{
public:
    /**
     * Creates a new KLimitedIODevice.
     * @param dev the underlying device, opened or not
     * This device itself auto-opens (in readonly mode), no need to open it.
     * @param start where to start reading (position in bytes)
     * @param length the length of the data to read (in bytes)
     */
    KLimitedIODevice(QIODevice *dev, qint64 start, qint64 length);
    virtual ~KLimitedIODevice()
    {
    }

    bool isSequential() const Q_DECL_OVERRIDE;

    bool open(QIODevice::OpenMode m) Q_DECL_OVERRIDE;
    void close() Q_DECL_OVERRIDE;

    qint64 size() const Q_DECL_OVERRIDE;

    qint64 readData(char *data, qint64 maxlen) Q_DECL_OVERRIDE;
    qint64 writeData(const char *, qint64) Q_DECL_OVERRIDE {
        return -1;    // unsupported
    }

    //virtual qint64 pos() const { return m_dev->pos() - m_start; }
    bool seek(qint64 pos) Q_DECL_OVERRIDE;
    qint64 bytesAvailable() const Q_DECL_OVERRIDE;
private:
    QIODevice *m_dev;
    qint64 m_start;
    qint64 m_length;
};

#endif
