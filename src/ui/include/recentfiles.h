/*
 * Copyright (C) 2011 Nicolas Bonnefon and other contributors
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

#ifndef RECENTFILES_H
#define RECENTFILES_H

#include <QString>
#include <QStringList>

#include "persistable.h"

// Manage the list of recently opened files
class RecentFiles final : public Persistable<RecentFiles, session_settings> {
  public:
    static const char* persistableName()
    {
        return "RecentFiles";
    }

    void addRecent( const QString& text );
    void removeRecent( const QString& text );
    void removeAll();
    int  getNumberItemsToShow() const;
    int  filesHistoryMaxItems() const;
    void setFilesHistoryMaxItems( const int recentFilesItems );

    // Returns a list of recent files (latest loaded first)
    QStringList recentFiles() const;

    void saveToStorage( QSettings& settings ) const;
    void retrieveFromStorage( QSettings& settings );

  private:
    static constexpr int RECENTFILES_VERSION = 1;
    static constexpr int DEFAULT_MAX_ITEMS_TO_SHOW = 5;

    QStringList recentFiles_;
    int filesHistoryMaxItemsToShow_ = DEFAULT_MAX_ITEMS_TO_SHOW;
};

#endif
