/* This file is part of the KDE libraries
   Copyright (C) 2002 Laurence Anderson <l.d.anderson@warwick.ac.uk>

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
#ifndef KAR_H
#define KAR_H

#include <karchive.h>

/**
 * @class KAr kar.h KAr
 *
 * KAr is a class for reading archives in ar format. Writing
 * is not supported. Reading archives that contain files bigger than
 * INT_MAX - 32 bytes is not supported.
 * @short A class for reading ar archives.
 * @author Laurence Anderson <l.d.anderson@warwick.ac.uk>
 */
class KARCHIVE_EXPORT KAr : public KArchive
{
    Q_DECLARE_TR_FUNCTIONS(KAr)

public:
    /**
     * Creates an instance that operates on the given filename.
     *
     * @param filename is a local path (e.g. "/home/holger/myfile.ar")
     */
    KAr(const QString &filename);

    /**
     * Creates an instance that operates on the given device.
     * The device can be compressed (KFilterDev) or not (QFile, etc.).
     * @param dev the device to read from
     */
    KAr(QIODevice *dev);

    /**
     * If the ar file is still opened, then it will be
     * closed automatically by the destructor.
     */
    virtual ~KAr();

protected:

    /*
     * Writing is not supported by this class, will always fail.
     * @return always false
     */
    bool doPrepareWriting(const QString &name, const QString &user, const QString &group, qint64 size,
                          mode_t perm, const QDateTime &atime, const QDateTime &mtime, const QDateTime &ctime) override;

    /*
     * Writing is not supported by this class, will always fail.
     * @return always false
     */
    bool doFinishWriting(qint64 size) override;

    /*
     * Writing is not supported by this class, will always fail.
     * @return always false
     */
    bool doWriteDir(const QString &name, const QString &user, const QString &group,
                    mode_t perm, const QDateTime &atime, const QDateTime &mtime, const QDateTime &ctime) override;

    bool doWriteSymLink(const QString &name, const QString &target,
                        const QString &user, const QString &group, mode_t perm,
                        const QDateTime &atime, const QDateTime &mtime, const QDateTime &ctime) override;

    /**
     * Opens the archive for reading.
     * Parses the directory listing of the archive
     * and creates the KArchiveDirectory/KArchiveFile entries.
     *
     */
    bool openArchive(QIODevice::OpenMode mode) override;
    bool closeArchive() override;

protected:
    void virtual_hook(int id, void *data) override;
private:
    class KArPrivate;
    KArPrivate *const d;
};

#endif
