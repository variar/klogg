/*
 * Copyright (C) 2013, 2014 Nicolas Bonnefon and other contributors
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

#include "session.h"

#include "log.h"

#include <cassert>
#include <QFileInfo>
#include <algorithm>

#include "viewinterface.h"
#include "persistentinfo.h"
#include "savedsearches.h"
#include "sessioninfo.h"
#include "logdata.h"
#include "logfiltereddata.h"

Session::Session()
{
    GetPersistentInfo().retrieve( "savedSearches" );

    // Get the global search history (it remains the property
    // of the Persistent)
    savedSearches_ = &(Persistent<SavedSearches>( "savedSearches" ));

    quickFindPattern_ = std::make_shared<QuickFindPattern>();
}

Session::~Session()
{
    // FIXME Clean up all the data objects...
}

ViewInterface* Session::getViewIfOpen( const QString& file_name ) const
{
    auto result = std::find_if( openFiles_.begin(), openFiles_.end(),
            [&](const std::pair<const ViewInterface*, OpenFile>& o)
            { return ( o.second.fileName == file_name ); } );

    if ( result != openFiles_.end() )
        return result->second.view;
    else
        return nullptr;
}

ViewInterface* Session::open( const QString& file_name,
        const std::function<ViewInterface*()>& view_factory )
{
    QFileInfo fileInfo( file_name );
    if ( fileInfo.isReadable() ) {
        return openAlways( file_name, view_factory, nullptr );
    }
    else {
        throw FileUnreadableErr();
    }
}

void Session::close( const ViewInterface* view )
{
    openFiles_.erase( openFiles_.find( view ) );
}

void Session::save( const std::vector<
        std::tuple<const ViewInterface*,
            uint64_t,
            std::shared_ptr<const ViewContextInterface>>
        >& view_list,
        const QByteArray& geometry )
{
    LOG(logDEBUG) << "Session::save";

    std::vector<SessionInfo::OpenFile> session_files;
    for ( const auto& view: view_list ) {
        const ViewInterface* view_object;
        uint64_t top_line;
        std::shared_ptr<const ViewContextInterface> view_context;

        std::tie( view_object, top_line, view_context ) = view;

        const OpenFile* file = findOpenFileFromView( view_object );
        assert( file );

        LOG(logDEBUG) << "Saving " << file->fileName.toLocal8Bit().data() << " in session.";
        session_files.emplace_back( file->fileName, top_line, view_context->toString() );
    }

    auto& session = Persistent<SessionInfo>( "session" );
    session.setOpenFiles( session_files );
    session.setGeometry( geometry );
    GetPersistentInfo().save( "session" );
}

std::vector<std::pair<QString, ViewInterface*>> Session::restore(
        const std::function<ViewInterface*()>& view_factory,
        int *current_file_index )
{
    GetPersistentInfo().retrieve( "session" );
    const auto& session = Persistent<SessionInfo>( "session" );

    std::vector<SessionInfo::OpenFile> session_files = session.openFiles();
    LOG(logDEBUG) << "Session returned " << session_files.size();
    std::vector<std::pair<QString, ViewInterface*>> result;

    for ( auto file: session_files )
    {
        LOG(logDEBUG) << "Create view for " << file.fileName.toLocal8Bit().data();
        ViewInterface* view = openAlways( file.fileName, view_factory, file.viewContext );
        result.emplace_back( file.fileName, view );
    }

    *current_file_index = -1;

    return result;
}

void Session::storedGeometry( QByteArray* geometry ) const
{
    GetPersistentInfo().retrieve( "session" );
    const auto& session = Persistent<SessionInfo>( "session" );

    *geometry = session.geometry();
}

QString Session::getFilename( const ViewInterface* view ) const
{
    const OpenFile* file = findOpenFileFromView( view );

    assert( file );

    return file->fileName;
}

void Session::getFileInfo( const ViewInterface* view, uint64_t* fileSize,
        uint32_t* fileNbLine, QDateTime* lastModified ) const
{
    const OpenFile* file = findOpenFileFromView( view );

    assert( file );

    *fileSize = file->logData->getFileSize();
    *fileNbLine = file->logData->getNbLine().get();
    *lastModified = file->logData->getLastModifiedDate();
}


/*
 * Private methods
 */

ViewInterface* Session::openAlways(const QString& file_name,
        const std::function<ViewInterface*()>& view_factory,
        const QString& view_context )
{
    // Create the data objects
    auto log_data          = std::make_shared<LogData>();
    auto log_filtered_data =
        std::shared_ptr<LogFilteredData>( log_data->getNewFilteredData() );

    ViewInterface* view = view_factory();
    view->setData( log_data, log_filtered_data );
    view->setQuickFindPattern( quickFindPattern_ );
    view->setSavedSearches( savedSearches_ );

    if ( !view_context.isEmpty() )
        view->setViewContext( view_context );

    // Insert in the hash
    openFiles_.insert( { view,
            { file_name,
            log_data,
            log_filtered_data,
            view } } );

    // Start loading the file
    log_data->attachFile( file_name );

    return view;
}

Session::OpenFile* Session::findOpenFileFromView( const ViewInterface* view )
{
    assert( view );

    OpenFile* file = &( openFiles_.at( view ) );

    // OpenfileMap::at might throw out_of_range but since a view MUST always
    // be attached to a file, we don't handle it!

    return file;
}

const Session::OpenFile* Session::findOpenFileFromView( const ViewInterface* view ) const
{
    assert( view );

    const OpenFile* file = &( openFiles_.at( view ) );

    // OpenfileMap::at might throw out_of_range but since a view MUST always
    // be attached to a file, we don't handle it!

    return file;
}
