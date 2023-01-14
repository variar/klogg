/*
 * Copyright (C) 2009, 2010 Nicolas Bonnefon and other contributors
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

// This file implements AbstractLogData.
// This base class is primarily an interface class and should not
// implement anything.
// It exists so that AbstractLogView can manipulate an abtract set of data
// (either full or filtered).

#include "abstractlogdata.h"

// Simple wrapper in order to use a clean Template Method
QString AbstractLogData::getLineString( LineNumber line ) const
{
    return doGetLineString( line );
}

// Simple wrapper in order to use a clean Template Method
QString AbstractLogData::getExpandedLineString( LineNumber line ) const
{
    return doGetExpandedLineString( line );
}

// Simple wrapper in order to use a clean Template Method
std::vector<QString> AbstractLogData::getLines( LineNumber first_line, LinesCount number ) const
{
    return doGetLines( first_line, number );
}

// Simple wrapper in order to use a clean Template Method
std::vector<QString> AbstractLogData::getExpandedLines( LineNumber first_line,
                                                        LinesCount number ) const
{
    return doGetExpandedLines( first_line, number );
}

LineNumber AbstractLogData::getLineNumber( LineNumber index ) const
{
    LineNumber ln = doGetLineNumber( index );
    // line number in file in editor starts from 1, convert it from 0 based index
    ln += LineNumber( 1 );
    return ln;
}

// Simple wrapper in order to use a clean Template Method
LinesCount AbstractLogData::getNbLine() const
{
    return doGetNbLine();
}

// Simple wrapper in order to use a clean Template Method
LineLength AbstractLogData::getMaxLength() const
{
    return doGetMaxLength();
}

// Simple wrapper in order to use a clean Template Method
LineLength AbstractLogData::getLineLength( LineNumber line ) const
{
    return doGetLineLength( line );
}

void AbstractLogData::setDisplayEncoding( const char* encoding )
{
    doSetDisplayEncoding( encoding );
}

QTextCodec* AbstractLogData::getDisplayEncoding() const
{
    return doGetDisplayEncoding();
}

void AbstractLogData::attachReader() const
{
    doAttachReader();
}

void AbstractLogData::detachReader() const
{
    doDetachReader();
}
