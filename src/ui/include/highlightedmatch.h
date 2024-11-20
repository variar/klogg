/*
 * Copyright (C) 2020 Anton Filimonov and other contributors
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

#ifndef KLOGG_HIGHLIGHTEDMATCH_H
#define KLOGG_HIGHLIGHTEDMATCH_H

#include "containers.h"
#include "linetypes.h"
#include <QColor>
#include <algorithm>

// Represents a match result for QuickFind or highlighter
class HighlightedMatch {
public:
    // Construct a match (must be initialised)
    HighlightedMatch( LineColumn start_column, LineLength size, QColor foreColor, QColor backColor )
        : startColumn_{ start_column }
        , size_{ size }
        , foreColor_{ foreColor }
        , backColor_{ backColor }
    {
    }

    LineColumn startColumn() const
    {
        return startColumn_;
    }

    LineColumn endColumn() const
    {
        return size_ > 0_length ? startColumn_ + size_ - 1_length : startColumn_;
    }

    LineLength size() const
    {
        return size_;
    }

    QColor foreColor() const
    {
        return foreColor_;
    }

    QColor backColor() const
    {
        return backColor_;
    }

private:
    LineColumn startColumn_;
    LineLength size_;

    QColor foreColor_;
    QColor backColor_;
};

class HighlightedMatchRanges {

public:
    HighlightedMatchRanges() = default;
    explicit HighlightedMatchRanges( klogg::vector<HighlightedMatch> matches )
        : matches_( std::move( matches ) )
    {
    }

    klogg::vector<HighlightedMatch> matches() const
    {
        return matches_;
    }

    void clear()
    {
        matches_.clear();
    }

    bool empty() const
    {
        return matches_.empty();
    }

    const HighlightedMatch& front() const
    {
        return matches_.front();
    }

    const HighlightedMatch& back() const
    {
        return matches_.back();
    }

    void clamp( LineColumn firstVisibleColumn, LineColumn lastVisibleColumn )
    {
        for ( HighlightedMatch& m : matches_ ) {
            if ( m.endColumn() < firstVisibleColumn || m.startColumn() > lastVisibleColumn ) {
                m = HighlightedMatch{ m.startColumn(), 0_length, m.foreColor(), m.backColor() };
                continue;
            }

            if ( m.startColumn() < firstVisibleColumn ) {
                m = HighlightedMatch{ firstVisibleColumn,
                                      m.endColumn() - firstVisibleColumn + 1_length, m.foreColor(),
                                      m.backColor() };
            }

            if ( m.endColumn() > lastVisibleColumn ) {
                m = HighlightedMatch{ m.startColumn(),
                                      lastVisibleColumn - m.startColumn() + 1_length, m.foreColor(),
                                      m.backColor() };
            }
        }
        matches_.erase(
            std::remove_if( matches_.begin(), matches_.end(),
                            []( const HighlightedMatch& m ) { return m.size() == 0_length; } ),
            matches_.end() );
    }

    void addMatches( const klogg::vector<HighlightedMatch>& patternMatches )
    {
        for ( HighlightedMatch m : patternMatches ) {
            addMatch( m );
        }
    }

    void addMatch( HighlightedMatch newMatch )
    {
        for ( auto matchIt = matches_.begin(); matchIt != matches_.end(); ++matchIt ) {
            HighlightedMatch& m = *matchIt;
            const LineColumn oldMatchL = m.startColumn();
            const LineColumn oldMatchR = m.endColumn();
            const LineColumn newMatchL = newMatch.startColumn();
            const LineColumn newMatchR = newMatch.endColumn();

            if ( oldMatchR < newMatchL ) {
                continue;
            }
            else if ( newMatchR < oldMatchL ) {
                break;
            }
            else if ( newMatchL <= oldMatchL && newMatchR >= oldMatchR ) {
                m = HighlightedMatch{ m.startColumn(), 0_length, m.foreColor(), m.backColor() };
            }
            else if ( oldMatchL <= newMatchL && oldMatchR >= newMatchR ) {
                m = HighlightedMatch{ m.startColumn(), newMatchL - oldMatchL, m.foreColor(),
                                      m.backColor() };

                HighlightedMatch tailMatch{ newMatchR + 1_length, oldMatchR - newMatchR,
                                            m.foreColor(), m.backColor() };
                matches_.insert( std::next( matchIt ), tailMatch );
                break;
            }
            else if ( oldMatchL < newMatchL && oldMatchR < newMatchR ) {
                m = HighlightedMatch{ m.startColumn(), newMatchL - oldMatchL, m.foreColor(),
                                      m.backColor() };
            }
            else if ( oldMatchL <= newMatchR && oldMatchR > newMatchR ) {
                m = HighlightedMatch{ newMatchR + 1_length, oldMatchR - newMatchR, m.foreColor(),
                                      m.backColor() };
                break;
            }
        }

        matches_.erase(
            std::remove_if( matches_.begin(), matches_.end(),
                            []( const HighlightedMatch& m ) { return m.size() == 0_length; } ),
            matches_.end() );

        auto insertPos
            = std::lower_bound( matches_.begin(), matches_.end(), newMatch,
                                []( const HighlightedMatch& lhs, const HighlightedMatch& rhs ) {
                                    return lhs.startColumn() < rhs.startColumn();
                                } );
        matches_.insert( insertPos, newMatch );
    }

private:
    klogg::vector<HighlightedMatch> matches_;
};

#endif // KLOGG_HIGHLIGHTEDMATCH_H
