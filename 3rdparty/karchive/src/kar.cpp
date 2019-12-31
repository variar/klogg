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

#include "kar.h"
#include "karchive_p.h"
#include "loggingcategory.h"

#include <QtCore/QFile>
#include <QtCore/QDebug>

#include "kfilterdev.h"
//#include "klimitediodevice_p.h"

////////////////////////////////////////////////////////////////////////
/////////////////////////// KAr ///////////////////////////////////////
////////////////////////////////////////////////////////////////////////

class Q_DECL_HIDDEN KAr::KArPrivate
{
public:
    KArPrivate()
    {
    }
};

KAr::KAr(const QString &filename)
    : KArchive(filename)
    , d(new KArPrivate)
{
}

KAr::KAr(QIODevice *dev)
    : KArchive(dev)
    , d(new KArPrivate)
{
}

KAr::~KAr()
{
    if (isOpen()) {
        close();
    }
    delete d;
}

bool KAr::doPrepareWriting(const QString &, const QString &, const QString &,
                           qint64, mode_t, const QDateTime &, const QDateTime &, const QDateTime &)
{
    setErrorString(tr("Cannot write to AR file"));
    qCWarning(KArchiveLog) << "doPrepareWriting not implemented for KAr";
    return false;
}

bool KAr::doFinishWriting(qint64)
{
    setErrorString(tr("Cannot write to AR file"));
    qCWarning(KArchiveLog) << "doFinishWriting not implemented for KAr";
    return false;
}

bool KAr::doWriteDir(const QString &, const QString &, const QString &,
                     mode_t, const QDateTime &, const QDateTime &, const QDateTime &)
{
    setErrorString(tr("Cannot write to AR file"));
    qCWarning(KArchiveLog) << "doWriteDir not implemented for KAr";
    return false;
}

bool KAr::doWriteSymLink(const QString &, const QString &, const QString &,
                         const QString &, mode_t, const QDateTime &, const QDateTime &, const QDateTime &)
{
    setErrorString(tr("Cannot write to AR file"));
    qCWarning(KArchiveLog) << "doWriteSymLink not implemented for KAr";
    return false;
}

bool KAr::openArchive(QIODevice::OpenMode mode)
{
    // Open archive

    if (mode == QIODevice::WriteOnly) {
        return true;
    }
    if (mode != QIODevice::ReadOnly && mode != QIODevice::ReadWrite) {
        setErrorString(tr("Unsupported mode %1").arg(mode));
        return false;
    }

    QIODevice *dev = device();
    if (!dev) {
        return false;
    }

    QByteArray magic = dev->read(7);
    if (magic != "!<arch>") {
        setErrorString(tr("Invalid main magic"));
        return false;
    }

    char *ar_longnames = nullptr;
    while (! dev->atEnd()) {
        QByteArray ar_header;
        ar_header.resize(61);

        dev->seek(dev->pos() + (2 - (dev->pos() % 2)) % 2);   // Ar headers are padded to byte boundary

        if (dev->read(ar_header.data(), 60) != 60) {   // Read ar header
            //qCWarning(KArchiveLog) << "Couldn't read header";
            delete[] ar_longnames;
            //return false;
            return true; // Probably EOF / trailing junk
        }

        if (!ar_header.endsWith("`\n")) { // Check header magic // krazy:exclude=strings
            setErrorString(tr("Invalid magic"));
            delete[] ar_longnames;
            return false;
        }

        QByteArray name = ar_header.mid(0, 16);   // Process header
        const int date = ar_header.mid(16, 12).toInt();
        //const int uid = ar_header.mid( 28, 6 ).toInt();
        //const int gid = ar_header.mid( 34, 6 ).toInt();
        const int mode = ar_header.mid(40, 8).toInt();
        const qint64 size = ar_header.mid(48, 10).toInt();

        bool skip_entry = false; // Deal with special entries
        if (name.mid(0, 1) == "/") {
            if (name.mid(1, 1) == "/") { // Longfilename table entry
                delete[] ar_longnames;
                ar_longnames = new char[size + 1];
                ar_longnames[size] = '\0';
                dev->read(ar_longnames, size);
                skip_entry = true;
                //qCDebug(KArchiveLog) << "Read in longnames entry";
            } else if (name.mid(1, 1) == " ") { // Symbol table entry
                //qCDebug(KArchiveLog) << "Skipped symbol entry";
                dev->seek(dev->pos() + size);
                skip_entry = true;
            } else { // Longfilename
                //qCDebug(KArchiveLog) << "Longfilename #" << name.mid(1, 15).toInt();
                if (! ar_longnames) {
                    setErrorString(tr("Invalid longfilename reference"));
                    delete[] ar_longnames;
                    return false;
                }
                name = &ar_longnames[name.mid(1, 15).toInt()];
                name = name.left(name.indexOf("/"));
            }
        }
        if (skip_entry) {
            continue;
        }

        name = name.trimmed(); // Process filename
        name.replace('/', QByteArray());
        //qCDebug(KArchiveLog) << "Filename: " << name << " Size: " << size;

        KArchiveEntry *entry = new KArchiveFile(this, QString::fromLocal8Bit(name.constData()), mode, KArchivePrivate::time_tToDateTime(date),
                                                rootDir()->user(), rootDir()->group(), /*symlink*/ QString(),
                                                dev->pos(), size);
        rootDir()->addEntry(entry); // Ar files don't support directories, so everything in root

        dev->seek(dev->pos() + size);   // Skip contents
    }
    delete[] ar_longnames;

    return true;
}

bool KAr::closeArchive()
{
    // Close the archive
    return true;
}

void KAr::virtual_hook(int id, void *data)
{
    KArchive::virtual_hook(id, data);
}
