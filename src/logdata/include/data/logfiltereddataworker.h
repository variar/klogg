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
#include <QThread>
#include <QMutex>
#include <QRegularExpression>

#include <QFuture>
#include <QFutureWatcher>

#include "atomicflag.h"
#include "linetypes.h"

class LogData;

// Class encapsulating a single matching line
// Contains the line number the line was found in and its content.
class MatchingLine {
  public:
    MatchingLine( LineNumber line ) : lineNumber_{ line }
    {}

    // Accessors
    LineNumber lineNumber() const { return lineNumber_; }

    bool operator <( const MatchingLine& other) const
    { return lineNumber_ < other.lineNumber_; }

  private:
    LineNumber lineNumber_;
};

// This is an array of matching lines.
// It shall be implemented for random lookup speed, so
// a fixed "in-place" array (vector) is probably fine.
using SearchResultArray = std::vector<MatchingLine>;

// This class is a mutex protected set of search result data.
// It is thread safe.
class SearchData
{
  public:
    SearchData() : maxLength_{0} { }

    // Atomically get all the search data
    // appending more results to passed array
    void getAll( LineLength* length, SearchResultArray* matches,
            LinesCount* nbLinesProcessed ) const;
    // Atomically set all the search data
    // (overwriting the existing)
    // (the matches are always moved)
    void setAll( LineLength length, SearchResultArray&& matches );
    // Atomically add to all the existing search data.
    void addAll( LineLength length, const SearchResultArray& matches, LinesCount nbLinesProcessed );
    // Get the number of matches
    LinesCount getNbMatches() const;
    // Get the last matched line number
    // That is "last" as in biggest, not latest
    // 0 if no matches have been found yet
    LineNumber getLastMatchedLineNumber() const;

    // Delete the match for the passed line (if it exist)
    void deleteMatch( LineNumber line );

    // Atomically clear the data.
    void clear();

  private:
    mutable QMutex dataMutex_;

    SearchResultArray matches_;
    LineLength maxLength_;
    LinesCount nbLinesProcessed_;
};

class SearchOperation : public QObject
{
  Q_OBJECT
  public:
    SearchOperation(const LogData& sourceLogData,
            const QRegularExpression &regExp,
            LineNumber startLine, LineNumber endLine );

    // Start the search operation, returns true if it has been done
    // and false if it has been cancelled (results not copied)
    virtual void start( SearchData& result ) = 0;

  signals:
    void searchProgressed( LinesCount nbMatches, int percent, LineNumber initialLine );

  public slots:
    void cancel();

  protected:
    // Implement the common part of the search, passing
    // the shared results and the line to begin the search from.
    void doSearch( SearchData& result, LineNumber initialLine );

    AtomicFlag interruptRequested_;
    const QRegularExpression regexp_;
    const LogData& sourceLogData_;
    LineNumber startLine_;
    LineNumber endLine_;
};

class FullSearchOperation : public SearchOperation
{
  Q_OBJECT
  public:
    FullSearchOperation( const LogData& sourceLogData, const QRegularExpression& regExp,
                         LineNumber startLine, LineNumber endLine )
        : SearchOperation( sourceLogData, regExp, startLine, endLine ) {}

    void start( SearchData& result ) override;
};

class UpdateSearchOperation : public SearchOperation
{
  Q_OBJECT
  public:
    UpdateSearchOperation( const LogData& sourceLogData, const QRegularExpression& regExp,
            LineNumber startLine, LineNumber endLine, LineNumber position )
        : SearchOperation( sourceLogData, regExp, startLine, endLine ),
        initialPosition_( position ) {}

    void start( SearchData& result ) override;

  private:
    LineNumber initialPosition_;
};

class LogFilteredDataWorker : public QObject
{
  Q_OBJECT

  public:
    explicit LogFilteredDataWorker( const LogData& sourceLogData );
    ~LogFilteredDataWorker() override;

    // Start the search with the passed regexp
    void search(const QRegularExpression &regExp, LineNumber startLine, LineNumber endLine );
    // Continue the previous search starting at the passed position
    // in the source file (line number)
    void updateSearch( const QRegularExpression& regExp, LineNumber startLine, LineNumber endLine, LineNumber position );

    // Interrupts the search if one is in progress
    void interrupt();

    // Returns a copy of the current indexing data
    void getSearchResult( LineLength* maxLength, SearchResultArray* searchMatches,
           LinesCount* nbLinesProcessed );

  signals:
    // Sent during the indexing process to signal progress
    // percent being the percentage of completion.
    void searchProgressed( LinesCount nbMatches, int percent, LineNumber initialLine );
    // Sent when indexing is finished, signals the client
    // to copy the new data back.
    void searchFinished();

    void searchCanceled();

  private:
    void connectSignalsAndRun(SearchOperation* operationRequested);
  private:
    const LogData& sourceLogData_;

    // Mutex to protect operationRequested_ and friends
    QMutex mutex_;
    QFuture<void> operationFuture_;
    QFutureWatcher<void> operationWatcher_;

    // Shared indexing data
    SearchData searchData_;
};

#endif
