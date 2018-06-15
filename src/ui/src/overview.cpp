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

#include "log.h"

#include "logfiltereddata.h"

#include "overview.h"

Overview::Overview() : matchLines_(), markLines_()
{
    logFilteredData_ = nullptr;
    height_          = 0;
    dirty_           = true;
    visible_         = false;
}

void Overview::setFilteredData( const LogFilteredData* logFilteredData )
{
    logFilteredData_ = logFilteredData;
}

void Overview::updateData( LinesCount totalNbLine )
{
    LOG(logDEBUG) << "OverviewWidget::updateData " << totalNbLine;

    linesInFile_ = totalNbLine;
    dirty_ = true;
}

void Overview::updateView( int height )
{
    // We don't touch the cache if the height hasn't changed
    if ( ( height != height_ ) || ( dirty_ == true ) ) {
        height_ = height;

        recalculatesLines();
    }
}

const std::vector<Overview::WeightedLine> *Overview::getMatchLines() const
{
    return &matchLines_;
}

const std::vector<Overview::WeightedLine> *Overview::getMarkLines() const
{
    return &markLines_;
}

std::pair<int,int> Overview::getViewLines() const
{
    int top = 0;
    int bottom = height_ - 1;

    if ( linesInFile_.get() > 0 ) {
        top = (int)((qint64)topLine_.get() * height_ / linesInFile_.get());
        bottom = (int)((qint64)top + nbLines_.get() * height_ / linesInFile_.get());
    }

    return std::pair<int,int>(top, bottom);
}

LineNumber Overview::fileLineFromY( int position ) const
{
    const auto line = static_cast<LineNumber::UnderlyingType>(
                (qint64)position * linesInFile_.get() / height_
            );

    return LineNumber(line);
}

int Overview::yFromFileLine( int file_line ) const
{
    int position = 0;

    if ( linesInFile_.get() > 0 )
        position =  (int)((qint64)file_line * height_ / linesInFile_.get());

    return position;
}

// Update the internal cache
void Overview::recalculatesLines()
{
    LOG(logDEBUG) << "OverviewWidget::recalculatesLines";

    if ( logFilteredData_ != nullptr ) {
        matchLines_.clear();
        markLines_.clear();

        if ( linesInFile_.get() > 0 ) {
            const auto nbLines = logFilteredData_->getNbLine();
            for ( auto i = 0_lnum; i < nbLines; ++i ) {
                LogFilteredData::FilteredLineType line_type =
                    logFilteredData_->filteredLineTypeByIndex( i );
                const auto line = logFilteredData_->getMatchingLineNumber( i );
                const auto position = static_cast<int>( (qint64)(line.get()) * height_ / linesInFile_.get() );
                if ( ( line_type & LogFilteredData::FilteredLineType::Match ) != LogFilteredData::FilteredLineType::None ) {
                    if ( ( ! matchLines_.empty() ) && matchLines_.back().position() == position ) {
                        // If the line is already there, we increase its weight
                        matchLines_.back().load();
                    }
                    else {
                        // If not we just add it
                        matchLines_.emplace_back(  position );
                    }
                }
                else {
                    if ( ( ! markLines_.empty() ) && markLines_.back().position() == position ) {
                        // If the line is already there, we increase its weight
                        markLines_.back().load();
                    }
                    else {
                        // If not we just add it
                        markLines_.emplace_back( position );
                    }
                }
            }
        }
    }
    else
        LOG(logDEBUG) << "Overview::recalculatesLines: logFilteredData_ == NULL";

    dirty_ = false;
}
