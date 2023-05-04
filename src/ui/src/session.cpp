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

#include <QFileInfo>
#include <algorithm>
#include <cassert>
#include <cstdint>

#include "logdata.h"
#include "logfiltereddata.h"
#include "savedsearches.h"
#include "sessioninfo.h"
#include "viewinterface.h"

Session::Session()
{
    // Get the global search history (it remains the property
    // of the Persistent)
    savedSearches_ = &SavedSearches::getSynced();
    SessionInfo::getSynced();

    quickFindPattern_ = std::make_shared<QuickFindPattern>();
}

Session::~Session()
{
    // FIXME Clean up all the data objects...
}

ViewInterface* Session::getViewIfOpen( const QString& file_name ) const
{
    auto result = std::find_if( openFiles_.begin(), openFiles_.end(),
                                [&]( const std::pair<const ViewInterface*, OpenFile>& o ) {
                                    return ( o.second.fileName == file_name );
                                } );

    if ( result != openFiles_.end() )
        return result->second.view;
    else
        return nullptr;
}

ViewInterface* Session::open( const QString& file_name,
                              const std::function<ViewInterface*()>& view_factory )
{
    return openAlways( file_name, view_factory, nullptr );
}

void Session::close( const ViewInterface* view )
{
    openFiles_.erase( openFiles_.find( view ) );
}

QString Session::getFilename( const ViewInterface* view ) const
{
    const OpenFile* file = findOpenFileFromView( view );

    assert( file );

    return file->fileName;
}

void Session::getFileInfo( const ViewInterface* view, uint64_t* fileSize, uint64_t* fileNbLine,
                           QDateTime* lastModified ) const
{
    const OpenFile* file = findOpenFileFromView( view );

    assert( file );

    *fileSize = static_cast<uint64_t>( file->logData->getFileSize() );
    *fileNbLine = file->logData->getNbLine().get();
    *lastModified = file->logData->getLastModifiedDate();
}

ViewInterface* Session::openAlways( const QString& file_name,
                                    const std::function<ViewInterface*()>& view_factory,
                                    const QString& view_context )
{
    // Create the data objects
    auto log_data = std::make_shared<LogData>();
    auto log_filtered_data = std::shared_ptr<LogFilteredData>( log_data->getNewFilteredData() );

    ViewInterface* view = view_factory();
    view->setData( log_data, log_filtered_data );
    view->setQuickFindPattern( quickFindPattern_ );
    view->setSavedSearches( savedSearches_ );

    if ( !view_context.isEmpty() )
        view->setViewContext( view_context );

    // Insert in the hash
    openFiles_.insert( { view, { file_name, log_data, log_filtered_data, view } } );

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

std::vector<WindowSession> Session::windowSessions()
{
    const auto& session = SessionInfo::getSynced();
    const auto& sessionWindows = session.windows();

    std::vector<WindowSession> windows;
    for ( auto i = 0; i < sessionWindows.size(); ++i ) {
        windows.emplace_back( shared_from_this(), sessionWindows.at( i ), i );
    }

    return windows;
}

void WindowSession::save(
    const std::vector<std::tuple<const ViewInterface*, uint64_t,
                                 std::shared_ptr<const ViewContextInterface>>>& view_list,
    const QByteArray& geometry )
{
    LOG_DEBUG << "Session::save";

    std::vector<SessionInfo::OpenFile> session_files;
    for ( const auto& view : view_list ) {
        const ViewInterface* view_object;
        uint64_t top_line;
        std::shared_ptr<const ViewContextInterface> view_context;

        std::tie( view_object, top_line, view_context ) = view;

        const Session::OpenFile* file = appSession_->findOpenFileFromView( view_object );
        assert( file );

        LOG_DEBUG << "Saving " << file->fileName.toLocal8Bit().data() << " in session.";
        session_files.emplace_back( file->fileName, top_line, view_context->toString() );
    }

    auto& session = SessionInfo::getSynced();
    session.setOpenFiles( windowId_, session_files );
    session.setGeometry( windowId_, geometry );
    session.save();
}

std::vector<std::pair<QString, ViewInterface*>>
WindowSession::restore( const std::function<ViewInterface*()>& view_factory,
                        int* current_file_index )
{
    const auto& session = SessionInfo::getSynced();

    std::vector<SessionInfo::OpenFile> session_files = session.openFiles( windowId_ );
    LOG_DEBUG << "Session returned " << session_files.size();
    std::vector<std::pair<QString, ViewInterface*>> result;

    for ( const auto& file : session_files ) {
        LOG_DEBUG << "Create view for " << file.fileName;
        ViewInterface* view
            = appSession_->openAlways( file.fileName, view_factory, file.viewContext );
        result.emplace_back( file.fileName, view );
        openedFiles_.emplace_back( file.fileName );
    }

    *current_file_index = static_cast<int>( result.size() - 1 );

    return result;
}

WindowSession::WindowSession( std::shared_ptr<Session> appSession, const QString& id, size_t index )
    : appSession_{ std::move( appSession ) }
    , windowId_{ id }
    , windowIndex_{ index }
{
    LOG_INFO << "created session for " << id;
    auto sessionInfo = SessionInfo::getSynced();
    sessionInfo.add( id );
    sessionInfo.save();
}

void WindowSession::restoreGeometry( QByteArray* geometry ) const
{
    const auto& session = SessionInfo::getSynced();
    *geometry = session.geometry( windowId_ );
}

bool WindowSession::close()
{
    LOG_INFO << "close window session " << windowId_;

    if ( appSession_->exitRequested() ) {
        return true;
    }

    auto& session = SessionInfo::getSynced();
    auto isRemoved = session.remove( windowId_ );
    session.save();

    LOG_INFO << "session is removed " << isRemoved;

    return !isRemoved;
}