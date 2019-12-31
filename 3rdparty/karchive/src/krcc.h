/* This file is part of the KDE libraries
   Copyright (C) 2014 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published
   by the Free Software Foundation; either version 2 of the License or
   ( at your option ) version 3 or, at the discretion of KDE e.V.
   ( which shall act as a proxy as in section 14 of the GPLv3 ), any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#ifndef KRCC_H
#define KRCC_H

#include <karchive.h>

/**
 * KRcc is a class for reading dynamic binary resources created by Qt's rcc tool
 * from a .qrc file and the files it points to.
 *
 * Writing is not supported.
 * @short A class for reading rcc resources.
 * @since 5.3
 */
class KARCHIVE_EXPORT KRcc : public KArchive
{
    Q_DECLARE_TR_FUNCTIONS(KRcc)

public:
    /**
     * Creates an instance that operates on the given filename.
     *
     * @param filename is a local path (e.g. "/home/holger/myfile.rcc")
     */
    KRcc(const QString &filename);

    /**
     * If the rcc file is still opened, then it will be
     * closed automatically by the destructor.
     */
    virtual ~KRcc();

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

    /*
     * Writing is not supported by this class, will always fail.
     * @return always false
     */
    bool doWriteSymLink(const QString &name, const QString &target,
                        const QString &user, const QString &group, mode_t perm,
                        const QDateTime &atime, const QDateTime &mtime, const QDateTime &ctime) override;

    /**
     * Registers the .rcc resource in the QResource system under a unique identifier,
     * then lists that, and creates the KArchiveFile/KArchiveDirectory entries.
     */
    bool openArchive(QIODevice::OpenMode mode) override;
    /**
     * Unregisters the .rcc resource from the QResource system.
     */
    bool closeArchive() override;

protected:
    void virtual_hook(int id, void *data) override;
private:
    class KRccPrivate;
    KRccPrivate *const d;
};

#endif
