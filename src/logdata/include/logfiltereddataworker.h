/*
 * Copyright (C) 2009, 2010 Nicolas Bonnefon and other contributors
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

#ifndef LOGFILTEREDDATAWORKERTHREAD_H
#define LOGFILTEREDDATAWORKERTHREAD_H

#include <QObject>

#include <qthreadpool.h>


#ifndef Q_MOC_RUN
#include <tbb/task_group.h>
#include <roaring.hh>
#include <roaring64map.hh>
#endif

#include "atomicflag.h"
#include "regularexpression.h"
#include "linetypes.h"
#include "synchronization.h"

class LogData;

// Class encapsulating a single matching line
// Contains the line number the line was found in and its content.
class MatchingLine {
  public:
    MatchingLine( LineNumber line )
        : lineNumber_{ line }
    {
    }

    // Accessors
    LineNumber lineNumber() const
    {
        return lineNumber_;
    }

    bool operator<( const MatchingLine& other ) const
    {
        return lineNumber_ < other.lineNumber_;
    }

  private:
    LineNumber lineNumber_;
};

// This is an array of matching lines.
// It shall be implemented for random lookup speed, so
// a fixed "in-place" array (vector) is probably fine.
using SearchResultArray = roaring::Roaring64Map;

struct SearchResults {
    SearchResultArray newMatches;
    LineLength maxLength;
    LinesCount processedLines;
};

// This class is a mutex protected set of search result data.
// It is thread safe.
class SearchData {
  public:
    // will clear new matches
    SearchResults takeCurrentResults() const;

    // Atomically add to all the existing search data.
    void addAll( const LineLength& length, const SearchResultArray& matches, LinesCount nbLinesProcessed );
    // Get the number of matches
    LinesCount getNbMatches() const;
    // Get the last matched line number
    // That is "last" as in biggest, not latest
    // 0 if no matches have been found yet
    LineNumber getLastMatchedLineNumber() const;

    LineNumber getLastProcessedLine() const;

    // Delete the match for the passed line (if it exist)
    void deleteMatch( LineNumber line );

    // Atomically clear the data.
    void clear();

  private:
    mutable SharedMutex dataMutex_;

    SearchResultArray matches_;
    mutable SearchResultArray newMatches_;
    LineLength maxLength_{ 0 };
    LinesCount nbLinesProcessed_{ 0 };
    LinesCount nbMatches_{ 0 };
};

class SearchOperation : public QObject {
    Q_OBJECT
  public:
    SearchOperation( const LogData& sourceLogData, AtomicFlag& interruptRequested,
                     const RegularExpressionPattern& regExp, LineNumber startLine,
                     LineNumber endLine );

    // Run the search operation, returns true if it has been done
    // and false if it has been cancelled (results not copied)
    virtual void run( SearchData& result ) = 0;

  Q_SIGNALS:
    void searchProgressed( LinesCount nbMatches, int percent, LineNumber initialLine );
    void searchFinished();

  protected:
    // Implement the common part of the search, passing
    // the shared results and the line to begin the search from.
    void doSearch( SearchData& result, LineNumber initialLine );

    AtomicFlag& interruptRequested_;
    const RegularExpressionPattern regexp_;
    const LogData& sourceLogData_;
    LineNumber startLine_;
    LineNumber endLine_;
};

class FullSearchOperation : public SearchOperation {
    Q_OBJECT
  public:
    FullSearchOperation( const LogData& sourceLogData, AtomicFlag& interruptRequested,
                         const RegularExpressionPattern& regExp, LineNumber startLine,
                         LineNumber endLine )
        : SearchOperation( sourceLogData, interruptRequested, regExp, startLine, endLine )
    {
    }

    void run( SearchData& result ) override;
};

class UpdateSearchOperation : public SearchOperation {
    Q_OBJECT
  public:
    UpdateSearchOperation( const LogData& sourceLogData, AtomicFlag& interruptRequested,
                           const RegularExpressionPattern& regExp, LineNumber startLine,
                           LineNumber endLine, LineNumber position )
        : SearchOperation( sourceLogData, interruptRequested, regExp, startLine, endLine )
        , initialPosition_( position )
    {
    }

    void run( SearchData& result ) override;

  private:
    LineNumber initialPosition_;
};

class LogFilteredDataWorker : public QObject {
    Q_OBJECT

  public:
    explicit LogFilteredDataWorker( const LogData& sourceLogData );
    ~LogFilteredDataWorker() noexcept override;

    LogFilteredDataWorker( const LogFilteredDataWorker& ) = delete;
    LogFilteredDataWorker& operator=( const LogFilteredDataWorker&& ) = delete;

    LogFilteredDataWorker( LogFilteredDataWorker&& ) = delete;
    LogFilteredDataWorker& operator=( LogFilteredDataWorker&& ) = delete;

    // Start the search with the passed regexp
    void search( const RegularExpressionPattern& regExp, LineNumber startLine, LineNumber endLine );
    // Continue the previous search starting at the passed position
    // in the source file (line number)
    void updateSearch( const RegularExpressionPattern& regExp, LineNumber startLine,
                       LineNumber endLine, LineNumber position );

    // Interrupts the search if one is in progress
    void interrupt();

    // get the current indexing data
    SearchResults getSearchResults() const;

  Q_SIGNALS:
    // Sent during the indexing process to signal progress
    // percent being the percentage of completion.
    void searchProgressed( LinesCount nbMatches, int percent, LineNumber initialLine );
    // Sent when indexing is finished, signals the client
    // to copy the new data back.
    void searchFinished();

  private:
    void connectSignalsAndRun( SearchOperation* operationRequested );

  private:
    const LogData& sourceLogData_;
    AtomicFlag interruptRequested_;

    QThreadPool operationsPool_;
    Mutex operationsMutex_;

    // Shared indexing data
    SearchData searchData_;
};

#endif
