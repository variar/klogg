/* This file is part of the KDE libraries
   Copyright (C) 2000, 2006 David Faure <faure@kde.org>

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

#include "kfilterdev.h"
#include "loggingcategory.h"
#include <config-compression.h>
#include <qmimedatabase.h>

#include <QDebug>

static KCompressionDevice::CompressionType findCompressionByFileName(const QString &fileName)
{
    if (fileName.endsWith(QLatin1String(".gz"), Qt::CaseInsensitive)) {
        return KCompressionDevice::GZip;
    }
#if HAVE_BZIP2_SUPPORT
    if (fileName.endsWith(QLatin1String(".bz2"), Qt::CaseInsensitive)) {
        return KCompressionDevice::BZip2;
    }
#endif
#if HAVE_XZ_SUPPORT
    if (fileName.endsWith(QLatin1String(".lzma"), Qt::CaseInsensitive) || fileName.endsWith(QLatin1String(".xz"), Qt::CaseInsensitive)) {
        return KCompressionDevice::Xz;
    }
#endif
    else {
        // not a warning, since this is called often with other mimetypes (see #88574)...
        // maybe we can avoid that though?
        //qCDebug(KArchiveLog) << "findCompressionByFileName : no compression found for " << fileName;
    }

    return KCompressionDevice::None;
}

KFilterDev::KFilterDev(const QString &fileName)
    : KCompressionDevice(fileName, findCompressionByFileName(fileName))
{
}

KCompressionDevice::CompressionType KFilterDev::compressionTypeForMimeType(const QString &mimeType)
{
    if (mimeType == QLatin1String("application/x-gzip")) {
        return KCompressionDevice::GZip;
    }
#if HAVE_BZIP2_SUPPORT
    if (mimeType == QLatin1String("application/x-bzip")
        || mimeType == QLatin1String("application/x-bzip2") // old name, kept for compatibility
       ) {
        return KCompressionDevice::BZip2;
    }
#endif
#if HAVE_XZ_SUPPORT
    if (mimeType == QLatin1String("application/x-lzma")    // legacy name, still used
        || mimeType == QLatin1String("application/x-xz")   // current naming
       ) {
        return KCompressionDevice::Xz;
    }
#endif
    QMimeDatabase db;
    const QMimeType mime = db.mimeTypeForName(mimeType);
    if (mime.isValid()) {
        if (mime.inherits(QStringLiteral("application/x-gzip"))) {
            return KCompressionDevice::GZip;
        }
#if HAVE_BZIP2_SUPPORT
        if (mime.inherits(QStringLiteral("application/x-bzip"))) {
            return KCompressionDevice::BZip2;
        }
#endif
#if HAVE_XZ_SUPPORT
        if (mime.inherits(QStringLiteral("application/x-lzma"))) {
            return KCompressionDevice::Xz;
        }

        if (mime.inherits(QStringLiteral("application/x-xz"))) {
            return KCompressionDevice::Xz;
        }
#endif
    }

    // not a warning, since this is called often with other mimetypes (see #88574)...
    // maybe we can avoid that though?
    //qCDebug(KArchiveLog) << "no compression found for" << mimeType;
    return KCompressionDevice::None;
}
