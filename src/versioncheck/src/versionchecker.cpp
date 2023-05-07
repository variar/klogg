/*
 * Copyright (C) 2014 Nicolas Bonnefon and other contributors
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

#include "versionchecker.h"
#include "configuration.h"
#include "log.h"

#include "klogg_version.h"

namespace {

#if defined( Q_OS_WIN )
static constexpr QLatin1String OsSuffix = QLatin1String( "-win", 4 );
#elif defined( Q_OS_OSX )
static constexpr QLatin1String OsSuffix = QLatin1String( "-osx", 4 );
#else
static constexpr QLatin1String OsSuffix = QLatin1String( "-linux", 6 );
#endif

static constexpr QLatin1String VERSION_URL
    = QLatin1String( "https://raw.githubusercontent.com/variar/klogg/master/latest.json", 65 );
static constexpr std::time_t CHECK_INTERVAL_S = 3600 * 24 * 7; /* 7 days */

bool isVersionNewer( const QString& current_version, const QString& new_version )
{
#if ( QT_VERSION >= QT_VERSION_CHECK( 6, 4, 0 ) )
    const auto parseVersion = []( const QString& version_string ) {
        qsizetype tweak_index = 0;
        auto version = QVersionNumber::fromString( QAnyStringView(version_string), &tweak_index );
        return std::make_pair( version, version_string.right( tweak_index + 1 ).toUInt() );
    };
#else
    const auto parseVersion = []( const QString& version_string ) {
        int tweak_index = 0;
        auto version = QVersionNumber::fromString( version_string, &tweak_index );
        return std::make_pair( version, version_string.right( tweak_index + 1 ).toUInt() );
    };
#endif

    const auto old = parseVersion( current_version );
    const auto next = parseVersion( new_version );

    return next > old;
}

} // namespace

void VersionCheckerConfig::retrieveFromStorage( QSettings& settings )
{
    LOG_DEBUG << "VersionCheckerConfig::retrieveFromStorage";

    if ( settings.contains( "VersionChecker/nextDeadline" ) )
        next_deadline_ = settings.value( "VersionChecker/nextDeadline" ).toLongLong();
}

void VersionCheckerConfig::saveToStorage( QSettings& settings ) const
{
    LOG_DEBUG << "VersionCheckerConfig::saveToStorage";

    settings.setValue( "VersionChecker/nextDeadline", static_cast<long long>( next_deadline_ ) );
}

VersionChecker::VersionChecker()
    : QObject()
    , manager_( new QNetworkAccessManager( this ) )
{
    manager_->setRedirectPolicy( QNetworkRequest::NoLessSafeRedirectPolicy );
}

void VersionChecker::startCheck()
{
    LOG_DEBUG << "VersionChecker::startCheck()";

    const auto& deadlineConfig = VersionCheckerConfig::getSynced();
    const auto& appConfig = Configuration::get();

    if ( appConfig.versionCheckingEnabled() ) {
        // Check the deadline has been reached
        if ( deadlineConfig.nextDeadline() < std::time( nullptr ) ) {
            connect( manager_, &QNetworkAccessManager::finished, this,
                     &VersionChecker::downloadFinished );

            LOG_DEBUG << "Requesting new version info from " << VERSION_URL;

            QNetworkRequest request;
            request.setUrl( QUrl( VERSION_URL ) );
            manager_->get( request );
        }
        else {
            LOG_DEBUG << "Deadline not reached yet, next check in "
                      << std::difftime( deadlineConfig.nextDeadline(), std::time( nullptr ) );
        }
    }
}

void VersionChecker::downloadFinished( QNetworkReply* reply )
{
    LOG_DEBUG << "VersionChecker::downloadFinished()";

    if ( reply->error() == QNetworkReply::NoError ) {
        const auto rawReply = reply->readAll();
        checkVersionData( rawReply );
    }
    else {
        LOG_WARNING << "Download failed: err " << reply->error();
    }

    reply->deleteLater();

    // Extend the deadline
    auto& config = VersionCheckerConfig::get();

    config.setNextDeadline( std::time( nullptr ) + CHECK_INTERVAL_S );

    config.save();
}

void VersionChecker::checkVersionData( QByteArray versionData )
{
    LOG_DEBUG << "Version reply: " << QString::fromUtf8( versionData );

    const auto latestJson = QJsonDocument::fromJson( versionData );
    const auto latestVersionMap = latestJson.toVariant().toMap();

    QString latestVersion;
    QString url;
    const auto stableVersions = latestVersionMap.value( "releases" ).toList();

    const auto currentVersion = kloggVersion();
    if ( std::any_of( stableVersions.begin(), stableVersions.end(),
                      [ &currentVersion ]( const auto& version ) {
                          return version.toString() == currentVersion;
                      } ) ) {
        latestVersion = latestVersionMap.value( "stable" ).toString();
        url = latestVersionMap.value( "stable_url" ).toString();
    }
    else {
        latestVersion = latestVersionMap.value( "ci" ).toString();
        url = latestVersionMap.value( "ci_url" ).toString() + OsSuffix;
    }

    const auto changeLog = latestVersionMap.value( "changelog" ).toList();

    QStringList changes;
    for ( const auto& entry : qAsConst( changeLog ) ) {
        const auto entryData = entry.toMap();
        const auto version = entryData.value( "version" ).toString();

        if ( isVersionNewer( currentVersion, version ) ) {
            changes
                << QString( "%1: %2" ).arg( version, entryData.value( "description" ).toString() );
        }
    }

    LOG_DEBUG << "Current version: " << currentVersion << ". Latest version is " << latestVersion
              << ", url " << url;
    if ( isVersionNewer( currentVersion, latestVersion ) ) {
        LOG_INFO << "Sending new version notification";

        Q_EMIT newVersionFound( latestVersion, url, changes );
    }
}