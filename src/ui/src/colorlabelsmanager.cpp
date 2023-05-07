/*
 * Copyright (C) 2021 Anton Filimonov and other contributors
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

#include "colorlabelsmanager.h"
#include "highlighterset.h"
#include <algorithm>
#include <cstddef>
#include <vector>

ColorLabelsManager::QuickHighlightersCollection ColorLabelsManager::colorLabels() const
{
    return quickHighlighters_;
}

ColorLabelsManager::QuickHighlightersCollection ColorLabelsManager::clear()
{
    for ( auto& quickHighlighters : quickHighlighters_ ) {
        quickHighlighters.clear();
    }
    currentLabel_.reset();
    
    return quickHighlighters_;
}

ColorLabelsManager::QuickHighlightersCollection
ColorLabelsManager::setColorLabel( size_t label, const QString& text )
{
    return updateColorLabel( label, text, true );
}

ColorLabelsManager::QuickHighlightersCollection
ColorLabelsManager::setNextColorLabel( const QString& text )
{
    const auto& quickHighlightersConfiguration
        = HighlighterSetCollection::get().quickHighlighters();

    std::vector<size_t> cycle;
    cycle.reserve( static_cast<size_t>( quickHighlightersConfiguration.size() ) );

    for ( auto i = 0; i < quickHighlightersConfiguration.size(); ++i ) {
        if ( quickHighlightersConfiguration[ i ].useInCycle ) {
            cycle.push_back( static_cast<size_t>( i ) );
        }
    }

    if ( cycle.empty() ) {
        return quickHighlighters_;
    }

    auto nextLabel = cycle.front();

    if ( currentLabel_.has_value() ) {
        auto nextIt = std::upper_bound( cycle.cbegin(), cycle.cend(), *currentLabel_ );
        if ( nextIt != cycle.cend() ) {
            nextLabel = *nextIt;
        }
    }

    currentLabel_ = nextLabel;

    return updateColorLabel( nextLabel, text, false );
}

ColorLabelsManager::QuickHighlightersCollection
ColorLabelsManager::updateColorLabel( size_t label, const QString& text, bool replaceCurrent )
{
    auto wasHighlightedAnyLabel = false;
    auto wasHighlightedOtherLabel = false;

    for ( auto i = 0u; i < quickHighlighters_.size(); ++i ) {
        const auto wasHighlighted = quickHighlighters_[ i ].removeAll( text ) != 0;
        wasHighlightedAnyLabel |= wasHighlighted;
        wasHighlightedOtherLabel |= ( wasHighlighted && i != label );
    }

    if ( !( wasHighlightedAnyLabel ) || ( wasHighlightedOtherLabel && replaceCurrent ) ) {
        quickHighlighters_[ label ].append( text );
    }

    return quickHighlighters_;
}