/*
 * Copyright (C) 2010 Nicolas Bonnefon and other contributors
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

// This file implements QuickFindPattern.
// This class implements part of the Quick Find mechanism, it only stores the
// current search pattern, once it has been confirmed (return pressed),
// it can be asked to return the matches in a specific string.

#include <iostream>

#include "quickfindpattern.h"

#include "configuration.h"

#include "quickfind.h"

constexpr Qt::GlobalColor QfForeColor = Qt::black;

bool QuickFindMatcher::isLineMatching( const QString& line, int column ) const
{
    if ( !isActive_ )
        return false;

    QRegularExpressionMatch match = regexp_.match( line, column );
    if ( match.hasMatch() ) {
        lastMatchStart_ = match.capturedStart();
        lastMatchEnd_ = match.capturedEnd() - 1;
        return true;
    }
    else {
        return false;
    }
}

bool QuickFindMatcher::isLineMatchingBackward( const QString& line, int column ) const
{
    if ( !isActive_ )
        return false;

    QRegularExpressionMatchIterator matches = regexp_.globalMatch( line );
    QRegularExpressionMatch lastMatch;
    while ( matches.hasNext() ) {
        QRegularExpressionMatch nextMatch = matches.peekNext();
        if ( column >= 0 && nextMatch.capturedEnd() >= column ) {
            break;
        }

        lastMatch = matches.next();
    }

    if ( lastMatch.hasMatch() ) {
        lastMatchStart_ = lastMatch.capturedStart();
        lastMatchEnd_ = lastMatch.capturedEnd() - 1;
        return true;
    }
    else {
        return false;
    }
}

void QuickFindMatcher::getLastMatch( qsizetype* start_col, qsizetype* end_col ) const
{
    *start_col = lastMatchStart_;
    *end_col = lastMatchEnd_;
}

void QuickFindPattern::changeSearchPattern( const QString& pattern, bool isRegex )
{

    // Determine the type of regexp depending on the config
    switch ( Configuration::get().quickfindRegexpType() ) {
    case SearchRegexpType::ExtendedRegexp:
        pattern_ = isRegex ? pattern : QRegularExpression::escape( pattern );
        break;
    default:
        pattern_ = pattern;
        break;
    }

    regexp_.setPattern( pattern_ );

    if ( regexp_.isValid() && ( !pattern_.isEmpty() ) )
        active_ = true;
    else
        active_ = false;

    emit patternUpdated();
}

void QuickFindPattern::changeSearchPattern( const QString& pattern, bool ignoreCase, bool isRegex )
{
    QRegularExpression::PatternOptions options = QRegularExpression::UseUnicodePropertiesOption;

    if ( ignoreCase )
        options |= QRegularExpression::CaseInsensitiveOption;

    regexp_.setPatternOptions( options );
    changeSearchPattern( pattern, isRegex );
}

bool QuickFindPattern::matchLine( const QString& line,
                                  std::vector<HighlightedMatch>& matches ) const
{
    matches.clear();

    if ( active_ ) {
        QRegularExpressionMatchIterator matchIterator = regexp_.globalMatch( line );
        const auto& config = Configuration::get();
        const auto backColor = config.qfBackColor();
        while ( matchIterator.hasNext() ) {
            QRegularExpressionMatch match = matchIterator.next();
            matches.emplace_back( match.capturedStart(), match.capturedLength(), QfForeColor,
                                  backColor );
        }
    }

    return ( !matches.empty() );
}

QuickFindMatcher QuickFindPattern::getMatcher() const
{
    return QuickFindMatcher( active_, regexp_ );
}
