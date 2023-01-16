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

    Portion( LineNumber line, int start_column, int end_column )
        : line_{ line }
        , startColumn_{ start_column }
        , endColumn_{ end_column }
    {
    }

    LineNumber line() const
    {
        return *line_;
    }
    int startColumn() const
    {
        return startColumn_;
    }
    int endColumn() const
    {
        return endColumn_;
    }

    bool isValid() const
    {
        return !!line_;
    }

    int length() const
    {
        return endColumn_ - startColumn_ + 1;
    }

  private:
    OptionalLineNumber line_;
    int startColumn_;
    int endColumn_;
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
    void selectPortion( LineNumber line, int start_column, int end_column );
    void selectPortion( const Portion& selection )
    {
        selectPortion( selection.line(), selection.startColumn(), selection.endColumn() );
    }
    // Select a range of lines (both start and end included)
    void selectRange( LineNumber start_line, LineNumber end_line );

    // Select a range from the previously selected line or beginning
    // of range (shift+click behaviour)
    void selectRangeFromPrevious( LineNumber line );

    // Crop selection so that in fit in the range ending with the line passed.
    void crop( LineNumber last_line );

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
    std::vector<LineNumber> getLines() const;

    // Returns wether the line passed is selected (entirely).
    bool isLineSelected( LineNumber line ) const;

    // Returns wether the line passed is selected in certain range.
    bool isPortionSelected( LineNumber line, int startColumn, int endColumn ) const;

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
    QList<std::pair<QString, LineNumber>>
    getSelectionWithLineNumbers( const AbstractLogData* logData ) const;

  private:
    // Line number currently selected, or -1 if none selected
    OptionalLineNumber selectedLine_;

    struct SelectedPartial {
        OptionalLineNumber line;
        int startColumn;
        int endColumn;
    };
    struct SelectedRange {
        // The limits of the range, sorted
        OptionalLineNumber startLine;
        LineNumber endLine;
        // The line selected first, used for shift+click
        LineNumber firstLine;

        LinesCount size() const
        {
            return LinesCount{ endLine.get() - startLine->get() + 1 };
        }
    };
    struct SelectedPartial selectedPartial_;
    struct SelectedRange selectedRange_;
};

#endif
