/*
 * Copyright (C) 2009, 2010, 2011, 2013 Nicolas Bonnefon
 * and other contributors
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

#ifndef LOGMAINVIEW_H
#define LOGMAINVIEW_H

#include "abstractlogview.h"
#include "data/logdata.h"

// Class implementing the main (top) view widget.
class LogMainView : public AbstractLogView
{
  Q_OBJECT
  public:
    LogMainView( const LogData* newLogData,
            const QuickFindPattern* const quickFindPattern,
            Overview* overview,
            OverviewWidget* overview_widget,
            QWidget* parent = nullptr );

    // Configure the view to use the passed filtered list
    // (used for couloured bullets)
    // Should be NULL or the empty LFD if no filtering is used
    void useNewFiltering( LogFilteredData* filteredData );

  protected:
    // Implements the virtual function
    LogData::LineType lineType( LineNumber lineNumber ) const override;

    void keyPressEvent( QKeyEvent* keyEvent ) override;

  private:
    LogFilteredData* filteredData_;
};

#endif
