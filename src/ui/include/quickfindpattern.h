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

#ifndef QUICKFINDPATTERN_H
#define QUICKFINDPATTERN_H

#include <QList>
#include <QObject>
#include <QRegularExpression>
#include <QString>
#include <qglobal.h>

#include "highlightedmatch.h"
#include "containers.h"
#include "linetypes.h"

class QuickFind;

class QuickFindMatcher {
  public:
    QuickFindMatcher() = default;

    QuickFindMatcher( bool isActive, const QRegularExpression& regexp )
        : isActive_( isActive )
        , regexp_{ regexp }
    {
    }

    bool isActive() const
    {
        return isActive_;
    }

    // Returns whether there is a match in the passed line, starting at
    // the passed column.
    // Results are stored internally.
    bool isLineMatching( const QString& line, const LineColumn& column = 0_lcol ) const;

    // Same as isLineMatching but search backward
    bool isLineMatchingBackward( const QString& line, const LineColumn& column = LineColumn{-1} ) const;

    // Must be called when isLineMatching returns 'true', returns
    // the position of the first match found.
    std::pair<LineColumn, LineColumn> getLastMatch() const;

  private:
    bool isActive_ = false;
    QRegularExpression regexp_;

    mutable LineColumn lastMatchStart_;
    mutable LineColumn lastMatchEnd_;
};

// Represents a search pattern for QuickFind (without its results)
class QuickFindPattern : public QObject {
    Q_OBJECT

  public:
    // Construct an empty search
    QuickFindPattern() = default;

    // Set the search to a new pattern, using the current
    // case status
    void changeSearchPattern( const QString& pattern, bool isRegex = false );

    // Set the search to a new pattern, as well as the case status
    void changeSearchPattern( const QString& pattern, bool ignoreCase, bool isRegex );

    // Returns whether the search is active (i.e. valid and non empty regexp)
    bool isActive() const
    {
        return active_;
    }

    // Return the text of the regex
    QString getPattern() const
    {
        return pattern_;
    }

    // Returns whether the passed line match the quick find search.
    // If so, it populate the passed list with the list of matches
    // within this particular line.
    bool matchLine( const QString& line, klogg::vector<HighlightedMatch>& matches ) const;

    QuickFindMatcher getMatcher() const;

  Q_SIGNALS:
    // Sent when the pattern is changed
    void patternUpdated();

  private:
    bool active_ = false;
    QRegularExpression regexp_;
    QString pattern_;
};

#endif
