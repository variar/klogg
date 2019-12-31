/* This file is part of the KDE libraries
   Copyright (C) 2002 Holger Schroeder <holger-kde@holgis.net>

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

#ifndef KZIPFILEENTRY_H
#define KZIPFILEENTRY_H

#include "karchive.h"

class KZip;
/**
 * @class KZipFileEntry kzipfileentry.h KZipFileEntry
 *
 * A KZipFileEntry represents a file in a zip archive.
 */
class KARCHIVE_EXPORT KZipFileEntry : public KArchiveFile
{
public:
    /**
     * Creates a new zip file entry. Do not call this, KZip takes care of it.
     */
    KZipFileEntry(KZip *zip, const QString &name, int access, const QDateTime &date,
                  const QString &user, const QString &group, const QString &symlink,
                  const QString &path, qint64 start, qint64 uncompressedSize,
                  int encoding, qint64 compressedSize);

    /**
     * Destructor. Do not call this.
     */
    ~KZipFileEntry();

    int encoding() const;
    qint64 compressedSize() const;

    /// Only used when writing
    void setCompressedSize(qint64 compressedSize);

    /// Header start: only used when writing
    void setHeaderStart(qint64 headerstart);
    qint64 headerStart() const;

    /// CRC: only used when writing
    unsigned long crc32() const;
    void setCRC32(unsigned long crc32);

    /// Name with complete path - KArchiveFile::name() is the filename only (no path)
    const QString &path() const;

    /**
     * @return the content of this file.
     * Call data() with care (only once per file), this data isn't cached.
     */
    QByteArray data() const Q_DECL_OVERRIDE;

    /**
     * This method returns a QIODevice to read the file contents.
     * This is obviously for reading only.
     * Note that the ownership of the device is being transferred to the caller,
     * who will have to delete it.
     * The returned device auto-opens (in readonly mode), no need to open it.
     */
    QIODevice *createDevice() const Q_DECL_OVERRIDE;

private:
    class KZipFileEntryPrivate;
    KZipFileEntryPrivate *const d;
};

#endif
