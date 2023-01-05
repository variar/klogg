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

#include <QFile>
#include <QSettings>

#include "log.h"
#include "recentfiles.h"
#include "configuration.h"

void RecentFiles::removeRecent( const QString& text )
{
    // First prune non existent files
    QMutableStringListIterator i( recentFiles_ );
    while ( i.hasNext() ) {
        if ( !QFile::exists( i.next() ) )
            i.remove();
    }

    recentFiles_.removeAll( text );
}

void RecentFiles::removeAll()
{
    recentFiles_.clear();
    recentFiles_.reserve( MAX_RECENT_FILES );
}
void RecentFiles::addRecent( const QString& text )
{

    // Remove any copy of the about to be added filename
    removeRecent( text );

    // Add at the front
    recentFiles_.push_front( text );

    // Trim the list if it's too long
    while ( recentFiles_.size() > MAX_RECENT_FILES )
        recentFiles_.pop_back();
}

int RecentFiles::getNumberItemsToShow() const
{
    return ( filesHistoryMaxItems_ < recentFiles_.count() ? filesHistoryMaxItems_ : recentFiles_.count() );
}

int RecentFiles::filesHistoryMaxItems() const
{
    return filesHistoryMaxItems_;
}

void RecentFiles::setFilesHistoryMaxItems( const int recentMaxFiles )
{
    if ( recentMaxFiles > 0 && recentMaxFiles <= MAX_RECENT_FILES ) {
        filesHistoryMaxItems_ = recentMaxFiles;
    }
}

QStringList RecentFiles::recentFiles() const
{
    return recentFiles_;
}

//
// Persistable virtual functions implementation
//

void RecentFiles::saveToStorage( QSettings& settings ) const
{
    LOG_DEBUG << "RecentFiles::saveToStorage";

    settings.beginGroup( "RecentFiles" );
    settings.setValue( "version", RECENTFILES_VERSION );
    settings.remove( "filesHistory" );
    settings.beginWriteArray( "filesHistory" );
    for ( int i = 0; i < recentFiles_.size(); ++i ) {
        settings.setArrayIndex( i );
        settings.setValue( "name", recentFiles_.at( i ) );
    }
    settings.endArray();
    settings.setValue( "maxMenuItems", filesHistoryMaxItems_ );
    settings.endGroup();
}

void RecentFiles::retrieveFromStorage( QSettings& settings )
{
    LOG_DEBUG << "RecentFiles::retrieveFromStorage";

    removeAll();

    if ( settings.contains( "RecentFiles/version" ) ) {
        settings.beginGroup( "RecentFiles" );
        if ( settings.value( "version" ).toInt() == RECENTFILES_VERSION ) {
            int size = settings.beginReadArray( "filesHistory" );
            size = ( size < MAX_RECENT_FILES ) ? size : MAX_RECENT_FILES ;
            for ( int i = 0; i < size; ++i ) {
                settings.setArrayIndex( i );
                QString file = settings.value( "name" ).toString();
                recentFiles_.append( file );
            }
            settings.endArray();
        }
        else {
            LOG_ERROR << "Unknown version of recent files, ignoring it...";
        }
        setFilesHistoryMaxItems( settings.value( "maxMenuItems", DEFAULT_MAX_ITEMS_TO_SHOW ).toInt() );
        settings.endGroup();
    }
}
