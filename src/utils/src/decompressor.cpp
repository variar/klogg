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

#include <QFileInfo>
#include <QtConcurrent>

#include "log.h"

#include "k7zip.h"
#include "ktar.h"
#include "kzip.h"

#include "kcompressiondevice.h"

#include "decompressor.h"

namespace {

enum class Archive { None, Zip7, Tar, Zip, Gz, Bz2, Xz };

Archive archiveType( const QString& archiveFilePath )
{
    const auto info = QFileInfo( archiveFilePath );
    const auto extension = info.suffix().toLower();
    LOG( logINFO ) << "Suffix is " << extension;

    if ( extension == "zip" ) {
        return Archive::Zip;
    }
    else if ( extension == "7z" ) {
        return Archive::Zip7;
    }
    else if ( extension == "tgz" || extension == "tbz2" || extension == "txz" ) {
        return Archive::Tar;
    }

    if ( extension == "gz" || extension == "bz2" || extension == "xz" ) {
        const auto completeSuffix = info.completeSuffix();
        if ( completeSuffix.contains( ".tar." ) ) {
            return Archive::Tar;
        }
        else if ( extension == "gz" ) {
            return Archive::Gz;
        }
        else if ( extension == "bz2" ) {
            return Archive::Bz2;
        }
        else if ( extension == "xz" ) {
            return Archive::Xz;
        }
    }

    return Archive::None;
}

std::shared_ptr<KArchive> makeExtractor( Archive archiveType, const QString& archiveFilePath )
{
    switch ( archiveType ) {
    case Archive::Zip:
        return std::make_shared<KZip>( archiveFilePath );
    case Archive::Zip7:
        return std::make_shared<K7Zip>( archiveFilePath );
    case Archive::Tar:
        return std::make_shared<KTar>( archiveFilePath );
    default:
        return {};
    }
}

std::shared_ptr<KCompressionDevice> makeDecompressor( Archive archiveType,
                                                      const QString& archiveFilePath )
{
    KCompressionDevice::CompressionType compression = KCompressionDevice::None;

    switch ( archiveType ) {
    case Archive::Gz:
        compression = KCompressionDevice::GZip;
        break;
    case Archive::Bz2:
        compression = KCompressionDevice::BZip2;
        break;
    case Archive::Xz:
        compression = KCompressionDevice::Xz;
        break;
    default:
        compression = KCompressionDevice::None;
    }

    if ( compression != KCompressionDevice::None ) {
        return std::make_shared<KCompressionDevice>( archiveFilePath, compression );
    }
    else {
        return {};
    }
}

} // namespace

Decompressor::Decompressor( QObject* parent )
    : QObject( parent )
{
    connect( &watcher_, &QFutureWatcher<bool>::finished, [this]() {
        LOG( logINFO ) << "Decompressor finished " << watcher_.result();
        emit finished( watcher_.result() );
    } );
}

DecompressAction Decompressor::action( const QString& archiveFilePath )
{
    const auto archive = archiveType( archiveFilePath );

    switch ( archive ) {
    case Archive::Zip:
    case Archive::Zip7:
    case Archive::Tar:
        return DecompressAction::Extract;
    case Archive::Gz:
    case Archive::Bz2:
    case Archive::Xz:
        return DecompressAction::Decompress;
    default:
        return DecompressAction::None;
    }
}

bool Decompressor::decompress( const QString& archiveFilePath, QFile* outputFile )
{
    auto decompressor = makeDecompressor( archiveType( archiveFilePath ), archiveFilePath );
    if ( !decompressor ) {
        LOG( logWARNING ) << "Unsupported archive " << archiveFilePath.constData();
        return false;
    }

    if ( !decompressor->open( QIODevice::ReadOnly ) ) {
        LOG( logWARNING ) << "Cannot open " << archiveFilePath.constData();
        return false;
    }

    future_ = QtConcurrent::run( [input = std::move( decompressor ), outputFile] {
        while ( !input->atEnd() ) {
            QByteArray data = input->read( 1024 * 1024 );
            outputFile->write( data );
        }
        input->close();
        outputFile->close();

        return true;
    } );

    watcher_.setFuture( future_ );

    return true;
}

bool Decompressor::extract( const QString& archiveFilePath, const QString& destination )
{
    auto archive = makeExtractor( archiveType( archiveFilePath ), archiveFilePath );
    if ( !archive ) {
        LOG( logWARNING ) << "Unsupported archive " << archiveFilePath.constData();
        return false;
    }

    // Open the archive
    if ( !archive->open( QIODevice::ReadOnly ) ) {
        LOG( logWARNING ) << "Cannot open " << archiveFilePath.constData();
        return false;
    }

    future_ = QtConcurrent::run( [ar = std::move( archive ), destination] {
        const KArchiveDirectory* root = ar->directory();
        const auto recursive = true;
        const auto result = root->copyTo( destination, recursive );
        ar->close();
        return result;
    } );

    watcher_.setFuture( future_ );

    return true;
}
