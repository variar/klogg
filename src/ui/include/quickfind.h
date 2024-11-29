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
 * Copyright (C) 2017 -- 2019 Anton Filimonov and other contributors
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

#ifndef QUICKFIND_H
#define QUICKFIND_H

#include <QFuture>
#include <QFutureWatcher>
#include <QObject>
#include <QPoint>
#include <QTime>

#include "atomicflag.h"
#include "linetypes.h"
#include "qfnotifications.h"
#include "quickfindpattern.h"
#include "selection.h"

class QuickFindPattern;
class AbstractLogData;

// Handle "long processing" notifications to the UI.
// reset() shall be called at the beginning of the search
// and then ping() should be called periodically during the processing.
// The notify() signal should be forwarded to the UI.
class SearchingNotifier : public QObject {
    Q_OBJECT

  public:
    SearchingNotifier()
        : dotToDisplay_{ 0 }
    {
    }

    // Reset internal timers at the beiginning of the processing
    void reset();
    // Shall be called frequently during processing, send the notification
    // and call the event loop when appropriate.
    // Pass the current line number and total number of line so that
    // a progress percentage is calculated and displayed.
    // (line shall be negative if ging in reverse)
    inline void ping( LineNumber line, LinesCount nb_lines, bool backward )
    {
        if ( startTime_.msecsTo( QTime::currentTime() ) > 1000 )
            sendNotification( line, nb_lines, backward );
    }

  Q_SIGNALS:
    // Sent when the UI shall display a message to the user.
    void notify( const QFNotification& message );

  private:
    void sendNotification( LineNumber current_line, LinesCount nb_lines, bool backward );

    QTime startTime_;
    int dotToDisplay_;
};

// Represents a search made with Quick Find (without its results)
// it keeps a pointer to a set of data and to a QuickFindPattern which
// are used for the searches. (the caller retains ownership of both).
class QuickFind : public QObject {
    Q_OBJECT

  public:
    // Construct a search
    explicit QuickFind( const AbstractLogData& logData );

    // Set the starting point that will be used by the next search
    void setSearchStartPoint( QPoint startPoint );

    // Make the object forget the 'no more match' flag.
    void resetLimits();

  public Q_SLOTS:
    // Used for incremental searches
    // Return the first occurrence of the passed pattern from the starting
    // point.  These searches don't change the starting point.
    void incrementallySearchForward( Selection selection, QuickFindMatcher matcher );
    void incrementallySearchBackward( Selection selection, QuickFindMatcher matcher );

    // Stop the currently ongoing incremental search, leave the selection
    // where it is if a match has been found, restore the old one
    // if not. Also throw away the start point associated with
    // the search.
    Selection incrementalSearchStop();

    // Throw away the current search and restore the initial
    // position/selection
    Selection incrementalSearchAbort();

    // Idem but ignore the direction and always search in the
    // specified direction
    void searchForward( Selection selection, QuickFindMatcher matcher );
    void searchBackward( Selection selection, QuickFindMatcher matcher );

    void stopSearch();

  Q_SIGNALS:
    // Sent when the UI shall display a message to the user.
    void notify( const QFNotification& message );
    // Sent when the UI shall clear the notification.
    void clearNotification();
    // Sent when search is completed
    void searchDone( bool hasMatch, Portion selection );

  private Q_SLOTS:
    void sendNotification( QFNotification notification );
    void onSearchFutureReady();

  private:
    enum QFDirection {
        None,
        Forward,
        Backward,
    };

    class LastMatchPosition {
      public:
        void set( LineNumber line, const LineColumn& column );
        void set( const FilePosition& position );
        void reset()
        {
            line_ = {};
            column_ = LineColumn{-1};
        }
        // Does the passed position come after the recorded one
        bool isLater( OptionalLineNumber line, const LineColumn& column ) const;
        bool isLater( const FilePosition& position ) const;
        // Does the passed position come before the recorded one
        bool isSooner( OptionalLineNumber line, const LineColumn& column ) const;
        bool isSooner( const FilePosition& position ) const;

      private:
        OptionalLineNumber line_;
        LineColumn column_{ -1 };
    };

    class IncrementalSearchStatus {
      public:
        /* Constructors */
        IncrementalSearchStatus() = default;

        IncrementalSearchStatus( QFDirection direction, const FilePosition& position,
                                 const Selection& initial_selection )
            : ongoing_( direction )
            , position_( position )
            , initialSelection_( initial_selection )
        {
        }

        bool isOngoing() const
        {
            return ( ongoing_ != None );
        }
        QFDirection direction() const
        {
            return ongoing_;
        }
        FilePosition position() const
        {
            return position_;
        }
        Selection initialSelection() const
        {
            return initialSelection_;
        }

      private:
        QFDirection ongoing_{ None };
        FilePosition position_;
        Selection initialSelection_;
    };

    // Pointers to external objects
    const AbstractLogData& logData_;

    // Owned objects

    // Position of the last match in the file
    // (to avoid searching multiple times where there is no result)
    LastMatchPosition lastMatch_;
    LastMatchPosition firstMatch_;

    SearchingNotifier searchingNotifier_;

    // Incremental search status
    IncrementalSearchStatus incrementalSearchStatus_;

    // Private functions
    Portion doSearchForward( const Selection& selection, const QuickFindMatcher& matcher );
    Portion doSearchForward( const FilePosition& start_position, const Selection& selection,
                             const QuickFindMatcher& matcher );
    Portion doSearchBackward( const Selection& selection, const QuickFindMatcher& matcher );
    Portion doSearchBackward( const FilePosition& start_position, const Selection& selection,
                              const QuickFindMatcher& matcher );

    AtomicFlag interruptRequested_;
    QFuture<Portion> operationFuture_;
    QFutureWatcher<Portion> operationWatcher_;
};

#endif
