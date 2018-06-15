/*
 * Copyright (C) 2009, 2010, 2012, 2017 Nicolas Bonnefon and other contributors
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

// This file implements the FilteredView concrete class.
// Most of the actual drawing and event management is done in AbstractLogView
// Only behaviour specific to the filtered (bottom) view is implemented here.

#include <cassert>

#include "filteredview.h"

FilteredView::FilteredView( LogFilteredData* newLogData,
        const QuickFindPattern* const quickFindPattern, QWidget* parent )
    : AbstractLogView( newLogData, quickFindPattern, parent )
{
    // We keep a copy of the filtered data for fast lookup of the line type
    logFilteredData_ = newLogData;
}

void FilteredView::setVisibility( Visibility visi )
{
    assert( logFilteredData_ );

    logFilteredData_->setVisibility( visi );

    updateData();
}

// For the filtered view, a line is always matching!
AbstractLogView::LineType FilteredView::lineType( LineNumber lineNumber ) const
{
    const auto type = logFilteredData_->filteredLineTypeByIndex( lineNumber );
    if ( ( type & LogFilteredData::FilteredLineType::Mark ) != LogFilteredData::FilteredLineType::None )
        return Marked;
    else
        return Match;
}

LineNumber FilteredView::displayLineNumber( LineNumber lineNumber ) const
{
    // Display a 1-based index
    return logFilteredData_->getMatchingLineNumber( lineNumber ) + 1_lcount;
}

LineNumber FilteredView::lineIndex(LineNumber lineNumber ) const
{
    return logFilteredData_->getLineIndexNumber( lineNumber );
}

LineNumber FilteredView::maxDisplayLineNumber() const
{
    return LineNumber( logFilteredData_->getNbTotalLines().get() );
}

void FilteredView::keyPressEvent( QKeyEvent* keyEvent )
{
    const auto noModifier = keyEvent->modifiers() == Qt::NoModifier;
    keyEvent->ignore();

    if ( noModifier ) {
        switch ( keyEvent->key() ) {
            case Qt::Key_BracketLeft:
            {
                auto i = getViewPosition() - 1_lcount;
                bool foundMark = false;
                for (; i != 0_lnum; --i ) {
                    if ( lineType( i ) == Marked ) {
                        foundMark = true;
                        break;
                    }
                }

                if ( !foundMark ) {
                    foundMark = lineType( i ) == Marked;
                }

                if ( foundMark ) {
                    selectAndDisplayLine( i );
                    keyEvent->accept();
                }

                break;
            }
            case Qt::Key_BracketRight:
            {
                const auto nbLines = logFilteredData_->getNbLine();
                for ( auto i = getViewPosition() + 1_lcount; i < nbLines; ++i ) {
                    if ( lineType( i ) == Marked ) {
                        selectAndDisplayLine( i );
                        break;
                    }
                }
                keyEvent->accept();
                break;
            }
        }
    }

    if ( !keyEvent->isAccepted() ) {
        AbstractLogView::keyPressEvent( keyEvent );
    }
}
