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

#ifndef VERSIONCHECKER_H
#define VERSIONCHECKER_H

#include <ctime>

#include <QObject>
#include <QtNetwork>

#include "persistable.h"

// This class holds the configuration options and persistent
// data for the version checker
class VersionCheckerConfig final : public Persistable<VersionCheckerConfig, session_settings> {
  public:
    static const char* persistableName()
    {
        return "VersionCheckerConfig";
    }
    std::time_t nextDeadline() const
    {
        return next_deadline_;
    }
    void setNextDeadline( std::time_t deadline )
    {
        next_deadline_ = deadline;
    }

    // Reads/writes the current config in the QSettings object passed
    virtual void saveToStorage( QSettings& settings ) const;
    virtual void retrieveFromStorage( QSettings& settings );

  private:
    std::time_t next_deadline_ = {};
};

// This class compares the current version number with the latest
// stored on a central server
class VersionChecker : public QObject {
    Q_OBJECT

  public:
    VersionChecker();
    ~VersionChecker() override = default;

    // Starts an asynchronous check for a newer version if it is needed.
    // A newVersionFound signal is sent if one is found.
    // In case of error or if no new version is found, no signal is emitted.
    void startCheck();

  Q_SIGNALS:
    // New version "version" is available
    void newVersionFound( const QString& version, const QString& url, const QStringList& changes );

  private Q_SLOTS:
    // Called when download is finished
    void downloadFinished( QNetworkReply* );

  private:
    void checkVersionData( const QByteArray& versionData );

  private:
    QNetworkAccessManager* manager_ = nullptr;
};

#endif
