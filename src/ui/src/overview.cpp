/*
 * Copyright (C) 2011, 2012 Nicolas Bonnefon and other contributors
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

// This file implements the Overview class.
// It provides support for drawing the match overview sidebar but
// the actual drawing is done in AbstractLogView which uses this class.

#include "linetypes.h"
#include "log.h"

#include "logfiltereddata.h"

#include "overview.h"

Overview::Overview()
    : matchLines_()
    , markLines_()
{
    logFilteredData_ = nullptr;
    height_ = 0;
    dirty_ = true;
    visible_ = false;
}

void Overview::setFilteredData( const LogFilteredData* logFilteredData )
{
    LOG_INFO << "OverviewWidget::setFilteredData " << (void*)logFilteredData;

    logFilteredData_ = logFilteredData;
    dirty_ = true;
}

void Overview::updateData( LinesCount totalNbLine )
{
    LOG_INFO << "OverviewWidget::updateData " << totalNbLine;

    linesInFile_ = totalNbLine;
    dirty_ = true;
}

void Overview::updateView( unsigned height )
{
    // We don't touch the cache if the height hasn't changed
    if ( ( height != height_ ) || ( dirty_ == true ) ) {
        height_ = height;

        recalculatesLines();
    }
}

const klogg::vector<Overview::WeightedLine>* Overview::getMatchLines() const
{
    return &matchLines_;
}

const klogg::vector<Overview::WeightedLine>* Overview::getMarkLines() const
{
    return &markLines_;
}

std::pair<int, int> Overview::getViewLines() const
{
    int top = 0;
    int bottom = static_cast<int>( height_ ) - 1;

    if ( linesInFile_.get() > 0 ) {
        top = static_cast<int>( ( topLine_.get() ) * height_ / ( linesInFile_.get() ) );

        bottom = top + static_cast<int>( nbLines_.get() * height_ / ( linesInFile_.get() ) );
    }

    return std::make_pair( top, bottom );
}

LineNumber Overview::fileLineFromY( int position ) const
{
    const auto line = static_cast<LineNumber::UnderlyingType>(
        static_cast<LineNumber::UnderlyingType>( position ) * linesInFile_.get() / static_cast<LineNumber::UnderlyingType>( height_ ) );

    return LineNumber{ line };
}

int Overview::yFromFileLine( LineNumber fileLine ) const
{
    int position = 0;

    if ( linesInFile_.get() > 0 )
        position = static_cast<int>( fileLine.get() * height_ / linesInFile_.get() );

    return position;
}

// Update the internal cache
void Overview::recalculatesLines()
{
    LOG_INFO << "OverviewWidget::recalculatesLines";

    if ( logFilteredData_ != nullptr ) {
        matchLines_.clear();
        markLines_.clear();

        if ( linesInFile_.get() > 0 ) {
            logFilteredData_->iterateOverLines( [ this ]( LineNumber line ) {
                const auto lineType = logFilteredData_->lineTypeByLine( line );
                const auto position = yFromFileLine( line );
                if ( lineType.testFlag( LogFilteredData::LineTypeFlags::Match ) ) {
                    if ( ( !matchLines_.empty() ) && matchLines_.back().position() == position ) {
                        // If the line is already there, we increase its weight
                        matchLines_.back().load();
                    }
                    else {
                        // If not we just add it
                        matchLines_.emplace_back( position );
                    }
                }
                else {
                    if ( ( !markLines_.empty() ) && markLines_.back().position() == position ) {
                        // If the line is already there, we increase its weight
                        markLines_.back().load();
                    }
                    else {
                        // If not we just add it
                        markLines_.emplace_back( position );
                    }
                }
            } );
        }
    }
    else
        LOG_INFO << "Overview::recalculatesLines: logFilteredData_ == NULL";

    dirty_ = false;
}
