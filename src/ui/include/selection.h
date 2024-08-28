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

#ifndef SELECTION_H
#define SELECTION_H

#include <QList>
#include <QString>
#include <cstddef>

#include "linetypes.h"

class AbstractLogData;

class Portion {
  public:
    Portion()
        : line_{}
        , startColumn_{ -1 }
        , endColumn_{ -1 }
    {
    }

    Portion( LineNumber line, LineColumn startColumn, LineColumn endColumn )
        : line_{ line }
        , startColumn_{ startColumn }
        , endColumn_{ endColumn }
    {
    }

    LineNumber line() const
    {
        return *line_;
    }
    LineColumn startColumn() const
    {
        return startColumn_;
    }
    LineColumn endColumn() const
    {
        return endColumn_;
    }

    bool isValid() const
    {
        return !!line_ && endColumn_ >= startColumn_ && startColumn_ >= 0_lcol;
    }

    LineLength size() const
    {
        return ( endColumn_ - startColumn_ ) + 1_length;
    }

  private:
    OptionalLineNumber line_;
    LineColumn startColumn_;
    LineColumn endColumn_;
};

// Represents a selection in an AbstractLogView
class Selection {
  public:
    // Construct an empty selection
    Selection();

    // Clear the selection
    void clear()
    {
        selectedPartial_.line = {};
        selectedLine_ = {};
    }

    // Select one line
    void selectLine( LineNumber line )
    {
        selectedPartial_.line = {};
        selectedRange_.startLine = {};
        selectedLine_ = line;
    }
    // Select a portion of line (both start and end included)
    void selectPortion( LineNumber line, LineColumn startColumn, LineColumn endColumn );
    void selectPortion( const Portion& selection )
    {
        selectPortion( selection.line(), selection.startColumn(), selection.endColumn() );
    }
    // Select a range of lines (both start and end included)
    void selectRange( LineNumber startLine, LineNumber endLine );

    // Select a range from the previously selected line or beginning
    // of range (shift+click behaviour)
    void selectRangeFromPrevious( LineNumber line );

    // Crop selection so that in fit in the range ending with the line passed.
    void crop( LineNumber lastLine );

    // Returns whether the selection is empty
    bool isEmpty() const
    {
        return ( !selectedPartial_.line ) && ( !selectedLine_ );
    }

    // Returns whether the selection is a single line
    bool isSingleLine() const
    {
        return !!selectedLine_;
    }

    // Returns whether the selection is a portion of line
    bool isPortion() const
    {
        return !!selectedPartial_.line;
    }

    // Returns whether the selection is a portion of line
    bool isPortion( LineNumber line ) const
    {
        return !!selectedPartial_.line && *selectedPartial_.line == line;
    }

    // Returns whether a portion is selected or not on the passed line.
    // If so, returns the portion position.
    Portion getPortionForLine( LineNumber line ) const;
    // Get a list of selected line(s), in order.
    klogg::vector<LineNumber> getLines() const;

    LinesCount getSelectedLinesCount() const;

    // Returns wether the line passed is selected (entirely).
    bool isLineSelected( LineNumber line ) const;

    // Returns wether the line passed is selected in certain range.
    bool isPortionSelected( LineNumber line, LineColumn startColumn, LineColumn endColumn ) const;

    // Returns the line selected or -1 if not a single line selection
    OptionalLineNumber selectedLine() const;

    // Returns the text selected from the passed AbstractLogData
    QString getSelectedText( const AbstractLogData* logData, bool lineNumbers = false ) const;

    // Return the position immediately after the current selection
    // (used for searches).
    // This is the next character or the start of the next line.
    FilePosition getNextPosition() const;

    // Idem from the position immediately before selection.
    FilePosition getPreviousPosition() const;

  private:
    std::vector<std::pair<LineNumber, QString>>
    getSelectionWithLineNumbers( const AbstractLogData* logData ) const;

  private:
    // Line number currently selected, or -1 if none selected
    OptionalLineNumber selectedLine_;

    struct SelectedPartial {
        OptionalLineNumber line;
        LineColumn startColumn;
        LineColumn endColumn;

        LineLength size() const
        {
            return ( endColumn - startColumn ) + 1_length;
        }
    };
    struct SelectedRange {
        // The limits of the range, sorted
        OptionalLineNumber startLine;
        LineNumber endLine;
        // The line selected first, used for shift+click
        LineNumber firstLine;

        LinesCount size() const
        {
            return startLine ? ( endLine - startLine.value() ) + 1_lcount : 0_lcount;
        }
    };
    struct SelectedPartial selectedPartial_;
    struct SelectedRange selectedRange_;
};

#endif
