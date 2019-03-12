/*
 * Copyright (C) 2010, 2013 Nicolas Bonnefon and other contributors
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

// This file implements QuickFind.
// This class implements the Quick Find mechanism using references
// to the logData, the QFP and the selection passed.
// Search is started just after the selection and the selection is updated
// if a match is found.

#include <QApplication>

#include "log.h"
#include "quickfindpattern.h"
#include "selection.h"
#include "abstractlogdata.h"

#include "quickfind.h"

void SearchingNotifier::reset()
{
    dotToDisplay_ = 0;
    startTime_ = QTime::currentTime();
}

void SearchingNotifier::sendNotification( qint64 current_line, qint64 nb_lines )
{
    LOG( logDEBUG ) << "Emitting Searching....";
    qint64 progress;
    if ( current_line < 0 )
        progress = ( nb_lines + current_line ) * 100 / nb_lines;
    else
        progress = current_line * 100 / nb_lines;
    emit notify( QFNotificationProgress( progress ) );

    QApplication::processEvents( QEventLoop::ExcludeUserInputEvents );
    startTime_ = QTime::currentTime().addMSecs( -800 );
}

void QuickFind::LastMatchPosition::set( LineNumber line, int column )
{
    if ( ( !line_.has_value() ) ||
            ( ( line <= *line_ ) && ( column < column_ ) ) )
    {
        line_ = line;
        column_ = column;
    }
}

void QuickFind::LastMatchPosition::set( const FilePosition &position )
{
    set( position.line(), position.column() );
}

bool QuickFind::LastMatchPosition::isLater( OptionalLineNumber line, int column ) const
{
    if ( !line_.has_value() || !line.has_value()  )
        return false;
    else if ( ( *line == *line_ ) && ( column >= column_ ) )
        return true;
    else if ( *line > *line_ )
        return true;
    else
        return false;
}

bool QuickFind::LastMatchPosition::isLater( const FilePosition &position ) const
{
    return isLater( position.line(), position.column() );
}

bool QuickFind::LastMatchPosition::isSooner( OptionalLineNumber line, int column ) const
{
    if ( !line_.has_value() || !line.has_value() )
        return false;
    else if ( ( *line == *line_ ) && ( column <= column_ ) )
        return true;
    else if ( *line < *line_ )
        return true;
    else
        return false;
}

bool QuickFind::LastMatchPosition::isSooner( const FilePosition &position ) const
{
    return isSooner( position.line(), position.column() );
}

QuickFind::QuickFind( const AbstractLogData* const logData,
        Selection* selection,
        const QuickFindPattern* const quickFindPattern ) :
    logData_( logData ), selection_( selection ),
    quickFindPattern_( quickFindPattern ),
    lastMatch_(), firstMatch_(), searchingNotifier_(),
    incrementalSearchStatus_()
{
    connect( &searchingNotifier_, &SearchingNotifier::notify,
            this, &QuickFind::notify );
}

void QuickFind::incrementalSearchStop()
{
    if ( incrementalSearchStatus_.isOngoing() ) {
        if ( selection_->isEmpty() ) {
            // Nothing found?
            // We reset the selection to what it was
            *selection_ = incrementalSearchStatus_.initialSelection();
        }

        incrementalSearchStatus_ = IncrementalSearchStatus();
    }
}

void QuickFind::incrementalSearchAbort()
{
    if ( incrementalSearchStatus_.isOngoing() ) {
        // We reset the selection to what it was
        *selection_ = incrementalSearchStatus_.initialSelection();
        incrementalSearchStatus_ = IncrementalSearchStatus();
    }
}

OptionalLineNumber QuickFind::incrementallySearchForward()
{
    LOG( logDEBUG ) << "QuickFind::incrementallySearchForward";

    // Position where we start the search from
    FilePosition start_position = selection_->getNextPosition();

    if ( incrementalSearchStatus_.direction() == Forward ) {
        // An incremental search is active, we restart the search
        // from the initial point
        LOG( logDEBUG ) << "Restart search from initial point";
        start_position = incrementalSearchStatus_.position();
    }
    else {
        // It's a new search so we search from the selection
        incrementalSearchStatus_ = IncrementalSearchStatus(
                Forward,
                start_position,
                *selection_ );
    }

    const auto line_found = doSearchForward( start_position );

    if ( line_found ) {
        // We have found a result...
        // ... the caller will jump to this line.
        return line_found;
    }
    else {
        // No result...
        // ... we want the client to show the initial line.
        selection_->clear();
        return incrementalSearchStatus_.position().line();
    }
}

OptionalLineNumber QuickFind::incrementallySearchBackward()
{
    LOG( logDEBUG ) << "QuickFind::incrementallySearchBackward";

    // Position where we start the search from
    FilePosition start_position = selection_->getPreviousPosition();

    if ( incrementalSearchStatus_.direction() == Backward ) {
        // An incremental search is active, we restart the search
        // from the initial point
        LOG( logDEBUG ) << "Restart search from initial point";
        start_position = incrementalSearchStatus_.position();
    }
    else {
        // It's a new search so we search from the selection
        incrementalSearchStatus_ = IncrementalSearchStatus(
                Backward,
                start_position,
                *selection_ );
    }

    const auto line_found = doSearchBackward( start_position );

    if ( line_found ) {
        // We have found a result...
        // ... the caller will jump to this line.
        return line_found;
    }
    else {
        // No result...
        // ... we want the client to show the initial line.
        selection_->clear();
        return incrementalSearchStatus_.position().line();
    }
}

OptionalLineNumber QuickFind::searchForward()
{
    incrementalSearchStatus_ = IncrementalSearchStatus();

    // Position where we start the search from
    FilePosition start_position = selection_->getNextPosition();

    return doSearchForward( start_position );
}


OptionalLineNumber QuickFind::searchBackward()
{
    incrementalSearchStatus_ = IncrementalSearchStatus();

    // Position where we start the search from
    FilePosition start_position = selection_->getPreviousPosition();

    return doSearchBackward( start_position );
}

// Internal implementation of forward search,
// returns the line where the pattern is found or -1 if not found.
// Parameters are the position the search shall start
OptionalLineNumber QuickFind::doSearchForward( const FilePosition &start_position )
{
    bool found = false;
    int found_start_col{};
    int found_end_col{};

    if ( ! quickFindPattern_->isActive() )
        return {};

    // Optimisation: if we are already after the last match,
    // we don't do any search at all.
    if ( lastMatch_.isLater( start_position ) ) {
        // Send a notification
        emit notify( QFNotificationReachedEndOfFile() );

        return {};
    }

    auto line = start_position.line();
    LOG( logDEBUG ) << "Start searching at line " << line;
    // We look at the rest of the first line
    if ( quickFindPattern_->isLineMatching(
                logData_->getExpandedLineString( line ),
                start_position.column() ) ) {
        quickFindPattern_->getLastMatch( &found_start_col, &found_end_col );
        found = true;
    }
    else {
        searchingNotifier_.reset();
        // And then the rest of the file
        const auto nb_lines = logData_->getNbLine();
        ++line;
        while ( line < nb_lines ) {
            if ( quickFindPattern_->isLineMatching(
                        logData_->getExpandedLineString( line ) ) ) {
                quickFindPattern_->getLastMatch(
                        &found_start_col, &found_end_col );
                found = true;
                break;
            }
            ++line;

            // See if we need to notify of the ongoing search
            searchingNotifier_.ping( line.get(), nb_lines.get() );
        }
    }

    if ( found ) {
        selection_->selectPortion(
                line, found_start_col, found_end_col );

        // Clear any notification
        emit clearNotification();

        return line;
    }
    else {
        // Update the position of the last match
        FilePosition last_match_position = selection_->getPreviousPosition();
        lastMatch_.set( last_match_position );

        // Send a notification
        emit notify( QFNotificationReachedEndOfFile() );

        return {};
    }
}

// Internal implementation of backward search,
// returns the line where the pattern is found or -1 if not found.
// Parameters are the position the search shall start
OptionalLineNumber QuickFind::doSearchBackward( const FilePosition &start_position )
{
    bool found = false;
    int start_col{};
    int end_col{};

    if ( ! quickFindPattern_->isActive() )
        return {};

    // Optimisation: if we are already before the first match,
    // we don't do any search at all.
    if ( firstMatch_.isSooner( start_position ) ) {
        // Send a notification
        emit notify( QFNotificationReachedBegininningOfFile() );

        return {};
    }

    auto line = start_position.line();
    LOG( logDEBUG ) << "Start searching at line " << line;
    // We look at the beginning of the first line
    if (    ( start_position.column() > 0 )
         && ( quickFindPattern_->isLineMatchingBackward(
                 logData_->getExpandedLineString( line ),
                 start_position.column() ) )
       ) {
        quickFindPattern_->getLastMatch( &start_col, &end_col );
        found = true;
    }
    else {
        searchingNotifier_.reset();
        // And then the rest of the file
        const auto nb_lines = logData_->getNbLine();
        if ( line.get() > 0) {
            --line;
            while ( true ) {
                if ( quickFindPattern_->isLineMatchingBackward(
                            logData_->getExpandedLineString( line ) ) ) {
                    quickFindPattern_->getLastMatch( &start_col, &end_col );
                    found = true;
                    break;
                }
                if(line.get() == 0 ) {
                    break;
                }

                --line;

                // See if we need to notify of the ongoing search
                searchingNotifier_.ping( -static_cast<qint64>( line.get() ), nb_lines.get() );
            }
        }
    }

    if ( found )
    {
        selection_->selectPortion( line, start_col, end_col );

        // Clear any notification
        emit clearNotification();

        return line;
    }
    else {
        // Update the position of the first match
        FilePosition first_match_position = selection_->getNextPosition();
        firstMatch_.set( first_match_position );

        // Send a notification
        LOG( logDEBUG ) << "QF: Send BOF notification.";
        emit notify( QFNotificationReachedBegininningOfFile() );

        return {};
    }
}

void QuickFind::resetLimits()
{
    lastMatch_.reset();
    firstMatch_.reset();
}
