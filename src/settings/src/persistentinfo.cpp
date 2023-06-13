/*
 * Copyright (C) 2011, 2014 Nicolas Bonnefon and other contributors
 *
 * This file is part of glogg.
 *
 * glogg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * glogg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with glogg.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Copyright (C) 2019 Anton Filimonov and other contributors
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

// Implements PersistentInfo, a singleton class which store/retrieve objects
// to persistent storage.

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

#include <whereami.h>

#include "log.h"
#include "uuid.h"

#include "persistentinfo.h"

constexpr uint8_t AppSettingsVersion = 1;
constexpr uint8_t SessionSettingsVersion = 2;

constexpr const char ApplicationSessionFile[] = "klogg";
constexpr const char SessionSettingsFile[] = "klogg_session";
constexpr const char PortableExtension[] = ".conf";

namespace {
QString makeSessionSettingsPath( const QString& appConfigPath )
{
    return QFileInfo( appConfigPath )
        .absoluteDir()
        .filePath( QString( SessionSettingsFile ) + PortableExtension );
}
} // namespace

PersistentInfo::PersistentInfo()
{
    QString executablePath;

    int dirnameLength = 0;
    const auto executablePathLength = wai_getExecutablePath( NULL, 0, &dirnameLength );
    if ( executablePathLength > 0 ) {
        auto path = std::vector<char>( static_cast<size_t>( executablePathLength ), '\0' );
        wai_getExecutablePath( &path[ 0 ], executablePathLength, &dirnameLength );
        executablePath = QString::fromUtf8( path.data(), dirnameLength );
    }

    const auto portableConfigPath
        = executablePath + QDir::separator() + ApplicationSessionFile + PortableExtension;

    LOG_INFO << "Portable config path " << portableConfigPath;

    const auto usePortableConfiguration = ForcePortable || QFileInfo::exists( portableConfigPath );

    if ( usePortableConfiguration ) {
        PreparePortableSettings( portableConfigPath );
    }
    else {
        PrepareOsSettings();
    }

    UpdateSettings();
}

void PersistentInfo::PreparePortableSettings( const QString& portableConfigPath )
{
    const auto sessionSettingsPath = makeSessionSettingsPath( portableConfigPath );

    if ( !QFileInfo::exists( sessionSettingsPath ) && QFileInfo::exists( portableConfigPath ) ) {
        QFile::copy( portableConfigPath, sessionSettingsPath );
    }

    appSettings_ = std::make_unique<QSettings>( portableConfigPath, QSettings::IniFormat );
    sessionSettings_ = std::make_unique<QSettings>( sessionSettingsPath, QSettings::IniFormat );
}

void PersistentInfo::PrepareOsSettings()
{
#ifdef Q_OS_WIN
    const auto format = QSettings::IniFormat;
#else
    const auto format = QSettings::NativeFormat;
#endif

    appSettings_ = std::make_unique<QSettings>( format, QSettings::UserScope, "klogg",
                                                ApplicationSessionFile );
    sessionSettings_
        = std::make_unique<QSettings>( format, QSettings::UserScope, "klogg", SessionSettingsFile );

#ifndef Q_OS_MAC
    const auto sessionSettingsPath = makeSessionSettingsPath( appSettings_->fileName() );

    if ( sessionSettings_->allKeys().isEmpty() ) {
        if ( QFile::exists( sessionSettingsPath ) ) {
            QSettings oldSessionSettings{ sessionSettingsPath, QSettings::IniFormat };
            for ( const auto& key : oldSessionSettings.allKeys() ) {
                sessionSettings_->setValue( key, oldSessionSettings.value( key ) );
            }
        }
        else {
            for ( const auto& key : appSettings_->allKeys() ) {
                sessionSettings_->setValue( key, appSettings_->value( key ) );
            }
        }
    }
#endif
}

void PersistentInfo::UpdateSettings()
{
    const auto oldAppSettingsVersion = appSettings_->value( "version", 0 ).toUInt();
    LOG_INFO << "App settings version" << oldAppSettingsVersion;

    if ( oldAppSettingsVersion != AppSettingsVersion ) {
        appSettings_->remove( "geometry" );
        appSettings_->remove( "versionchecker.nextDeadline" );
        appSettings_->remove( "OpenFiles" );
        appSettings_->remove( "RecentFiles" );
        appSettings_->remove( "SavedSearches" );
    }

    std::pair<QString, QString> keysToMoveAround[] = {
        { "DefaultConfigurationView.searchAutoRefresh", "defaultView.searchAutoRefresh" },
        { "DefaultConfigurationView.searchIgnoreCase", "defaultView.searchIgnoreCase" },
        { "DefaultConfigurationView.splitterSizes", "defaultView.splitterSizes" },
    };

    for ( const auto& keyToMove : keysToMoveAround ) {
        if ( appSettings_->contains( keyToMove.first ) ) {
            appSettings_->setValue( keyToMove.second, appSettings_->value( keyToMove.first ) );
            appSettings_->remove( keyToMove.first );
        }
    }

    const auto oldSessionSettingsVersion = sessionSettings_->value( "version", 0 ).toUInt();
    LOG_INFO << "Session settings version" << oldSessionSettingsVersion;

    if ( oldSessionSettingsVersion < 1 ) {
        sessionSettings_->setValue( "Window/geometry", sessionSettings_->value( "geometry" ) );
        sessionSettings_->setValue( "VersionChecker/nextDeadline",
                                    sessionSettings_->value( "versionchecker.nextDeadline" ) );
        sessionSettings_->remove( "HighlighterSet" );
        for ( const auto& key : sessionSettings_->childKeys() ) {
            sessionSettings_->remove( key );
        }
    }
    if ( oldSessionSettingsVersion < SessionSettingsVersion ) {
        const auto geometry = sessionSettings_->value( "Window/geometry" );
        sessionSettings_->remove( "Window/geometry" );

        if ( geometry.isValid() ) {
            const auto id = generateIdFromUuid();
            sessionSettings_->beginGroup( "OpenFiles" );
            std::vector<std::tuple<QString, uint64_t, QString>> openFiles;
            int size = sessionSettings_->beginReadArray( "openFiles" );
            LOG_INFO << "OpenFiles" << size;

            for ( int i = 0; i < size; ++i ) {
                sessionSettings_->setArrayIndex( i );
                QString fileName = sessionSettings_->value( "fileName" ).toString();
                uint64_t topLine = sessionSettings_->value( "topLine" ).toULongLong();
                QString viewContext = sessionSettings_->value( "viewContext" ).toString();
                openFiles.emplace_back( fileName, topLine, viewContext );
            }
            sessionSettings_->endArray();
            sessionSettings_->endGroup(); // OpenFiles

            sessionSettings_->beginGroup( "Window" );
            sessionSettings_->setValue( "version", 1 );

            sessionSettings_->beginWriteArray( "windows" );
            sessionSettings_->setArrayIndex( 0 );
            sessionSettings_->setValue( "id", id );
            sessionSettings_->setValue( "geometry", geometry );

            sessionSettings_->beginGroup( "OpenFiles" );
            sessionSettings_->setValue( "version", 1 );
            sessionSettings_->beginWriteArray( "openFiles" );
            for ( unsigned i = 0; i < openFiles.size(); ++i ) {
                sessionSettings_->setArrayIndex( static_cast<int>( i ) );

                sessionSettings_->setValue( "fileName", std::get<0>( openFiles.at( i ) ) );
                sessionSettings_->setValue( "topLine", qint64( std::get<1>( openFiles.at( i ) ) ) );
                sessionSettings_->setValue( "viewContext", std::get<2>( openFiles.at( i ) ) );
            }
            sessionSettings_->endArray(); // openfiles
            sessionSettings_->endGroup(); // OpenFiles
            sessionSettings_->endArray(); // windows
            sessionSettings_->endGroup(); // Window
        }
    }

    appSettings_->setValue( "version", AppSettingsVersion );
    sessionSettings_->setValue( "version", SessionSettingsVersion );
}

PersistentInfo& PersistentInfo::getInstance()
{
    static PersistentInfo pInfo;
    return pInfo;
}

QSettings& PersistentInfo::getSettings( app_settings )
{
    return *getInstance().appSettings_;
}

QSettings& PersistentInfo::getSettings( session_settings )
{
    return *getInstance().sessionSettings_;
}
