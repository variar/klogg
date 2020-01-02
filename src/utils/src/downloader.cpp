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

#include "downloader.h"
#include "log.h"

#include <QNetworkReply>

Downloader::Downloader( QObject* parent )
    : QObject( parent )
{
    manager_.setRedirectPolicy( QNetworkRequest::NoLessSafeRedirectPolicy );
}

void Downloader::download( const QUrl& url, QFile* outputFile )
{
    output_ = outputFile;

    QNetworkRequest request( url );
    currentDownload_ = manager_.get( request );

    connect( currentDownload_, &QNetworkReply::downloadProgress, this, &Downloader::downloadProgress );

    connect( currentDownload_, &QNetworkReply::finished, this, &Downloader::downloadFinished );

    connect( currentDownload_, &QNetworkReply::readyRead, this, &Downloader::downloadReadyRead );

    LOG( logINFO ) << "Downloading " << url.toEncoded().constData();
}

void Downloader::downloadFinished()
{
    output_->close();
    currentDownload_->deleteLater();

    if ( currentDownload_->error() ) {
        // download failed
        LOG( logERROR ) << "Download failed: " << currentDownload_->errorString();
        output_->remove();
        emit finished( false );
    }
    else {
        LOG( logINFO ) << "Download done";
        output_->close();
        emit finished( true );
    }
}

void Downloader::downloadReadyRead()
{
    output_->write( currentDownload_->readAll() );
}