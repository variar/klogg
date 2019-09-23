/*
 * Copyright (C) 2011 Nicolas Bonnefon and other contributors
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

#include "marks.h"

#include "log.h"

// This file implements the list of marks for a file.
// It is implemented as a QList which is kept in order when inserting,
// it is simpler than something fancy like a heap, and insertion are
// not done very often anyway.  Oh and we need to iterate through the
// list, disqualifying a straight heap.

Marks::Marks() : marks_()
{
}

uint32_t Marks::addMark( LineNumber line, QChar mark )
{
    // 'mark' is not used yet
    Q_UNUSED(mark);

    // Look for the index immediately before
    uint32_t index {0};
    if ( ! lookupLineNumber( marks_, line, index ) )
    {
        // If a mark is not already set for this line
        LOG(logDEBUG) << "Inserting mark at line " << line
            << " (index " << index << ")";
        marks_.emplace( marks_.begin() + index,  line );
    }
    else
    {
        LOG(logERROR) << "Trying to add an existing mark at line " << line;
    }

    return index;
}

LineNumber Marks::getMark( QChar mark ) const
{
    // 'mark' is not used yet
    Q_UNUSED(mark);

    return 0_lnum;
}

bool Marks::isLineMarked( LineNumber line ) const
{
    uint32_t index;
    return lookupLineNumber( marks_, line, index );
}

void Marks::deleteMark( QChar mark )
{
    // 'mark' is not used yet
    Q_UNUSED(mark);
}

uint32_t Marks::deleteMark( LineNumber line )
{
    uint32_t index;

    if ( lookupLineNumber( marks_, line, index ) )
    {
        marks_.erase( marks_.begin() + index );
    }

    return index;
}

bool Marks::toggleMark( LineNumber line, QChar mark, uint32_t &index )
{
    // 'mark' is not used yet
    Q_UNUSED(mark);

    // Look for the index immediately before
    if ( lookupLineNumber( marks_, line, index ) )
    {
        marks_.erase( marks_.begin() + index );
        return false;
    }
    else
    {
        // If a mark is not already set for this line
        LOG(logDEBUG) << "Inserting mark at line " << line
            << " (index " << index << ")";
        marks_.emplace( marks_.begin() + index,  line );
        return true;
    }
}

void Marks::clear()
{
    marks_.clear();
}
