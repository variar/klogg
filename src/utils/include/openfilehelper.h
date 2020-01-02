/*
 * Copyright (C) 2016 -- 2019 Anton Filimonov
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
 *
 * showPathInFileExplorer code is derived from Qt Creator sources
 */

/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <QtCore/QProcess>
#include <QtGui/QDesktopServices>



inline void showPathInFileExplorer( const QString& file_path )
{
    const QFileInfo file_info( file_path );

    LOG( logINFO ) << "Show path in explorer: " << file_path;

#if defined( Q_OS_WIN )
    const auto explorer = QString( "explorer.exe /select,%1" )
                              .arg( QDir::toNativeSeparators( file_info.canonicalFilePath() ) );
    QProcess::startDetached( explorer );
#elif defined( Q_OS_MAC )
    QStringList scriptArgs;
    scriptArgs << QLatin1String( "-e" )
               << QString::fromLatin1( "tell application \"Finder\" to reveal POSIX file \"%1\"" )
                      .arg( file_info.canonicalFilePath() );
    QProcess::execute( QLatin1String( "/usr/bin/osascript" ), scriptArgs );
    scriptArgs.clear();
    scriptArgs << QLatin1String( "-e" )
               << QLatin1String( "tell application \"Finder\" to activate" );
    QProcess::execute( QLatin1String( "/usr/bin/osascript" ), scriptArgs );
#else
    QDesktopServices::openUrl( QUrl::fromLocalFile( file_info.canonicalPath() ) );
#endif
}

inline void openFileInDefaultApplication( const QString& file_path )
{
    LOG( logINFO ) << "Open file in default app: " << file_path;

    const QFileInfo file_info( file_path );
    QDesktopServices::openUrl( QUrl::fromLocalFile( file_info.canonicalFilePath() ) );
}

