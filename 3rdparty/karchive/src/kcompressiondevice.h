/* This file is part of the KDE libraries
   Copyright (C) 2000 David Faure <faure@kde.org>
   Copyright (C) 2011 Mario Bensi <mbensi@ipsquad.net>

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
#ifndef __kcompressiondevice_h
#define __kcompressiondevice_h

#include "karchive_export.h"
#include <QtCore/QIODevice>
#include <QtCore/QString>
#include <QtCore/QMetaType>
class KCompressionDevicePrivate;

class KFilterBase;

/**
 * @class KCompressionDevice kcompressiondevice.h KCompressionDevice
 *
 * A class for reading and writing compressed data onto a device
 * (e.g. file, but other usages are possible, like a buffer or a socket).
 *
 * Use this class to read/write compressed files.
 */

class KARCHIVE_EXPORT KCompressionDevice : public QIODevice
{
public:
    enum CompressionType {
        GZip,
        BZip2,
        Xz,
        None
    };

    /**
     * Constructs a KCompressionDevice for a given CompressionType (e.g. GZip, BZip2 etc.).
     * @param inputDevice input device.
     * @param autoDeleteInputDevice if true, @p inputDevice will be deleted automatically
     * @param type the CompressionType to use.
     */
    KCompressionDevice(QIODevice *inputDevice, bool autoDeleteInputDevice, CompressionType type);

    /**
     * Constructs a KCompressionDevice for a given CompressionType (e.g. GZip, BZip2 etc.).
     * @param fileName the name of the file to filter.
     * @param type the CompressionType to use.
     */
    KCompressionDevice(const QString &fileName, CompressionType type);

    /**
     * Destructs the KCompressionDevice.
     * Calls close() if the filter device is still open.
     */
    virtual ~KCompressionDevice();

    /**
     * The compression actually used by this device.
     * If the support for the compression requested in the constructor
     * is not available, then the device will use None.
     */
    CompressionType compressionType() const;

    /**
     * Open for reading or writing.
     */
    bool open(QIODevice::OpenMode mode) Q_DECL_OVERRIDE;

    /**
     * Close after reading or writing.
     */
    void close() Q_DECL_OVERRIDE;

    /**
     * For writing gzip compressed files only:
     * set the name of the original file, to be used in the gzip header.
     * @param fileName the name of the original file
     */
    void setOrigFileName(const QByteArray &fileName);

    /**
     * Call this let this device skip the gzip headers when reading/writing.
     * This way KCompressionDevice (with gzip filter) can be used as a direct wrapper
     * around zlib - this is used by KZip.
     */
    void setSkipHeaders();

    /**
     * That one can be quite slow, when going back. Use with care.
     */
    bool seek(qint64) Q_DECL_OVERRIDE;

    bool atEnd() const Q_DECL_OVERRIDE;

    /**
     * Call this to create the appropriate filter for the CompressionType
     * named @p type.
     * @param type the type of the compression filter
     * @return the filter for the @p type, or 0 if not found
     */
    static KFilterBase *filterForCompressionType(CompressionType type);

protected:
    friend class K7Zip;

    qint64 readData(char *data, qint64 maxlen) Q_DECL_OVERRIDE;
    qint64 writeData(const char *data, qint64 len) Q_DECL_OVERRIDE;

    KFilterBase *filterBase();
private:
    KCompressionDevicePrivate *const d;
};

Q_DECLARE_METATYPE(KCompressionDevice::CompressionType)

#endif
