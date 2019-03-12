/*
 * Copyright (C) 2011, 2012, 2013 Nicolas Bonnefon and other contributors
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

#ifndef OVERVIEWWIDGET_H
#define OVERVIEWWIDGET_H

#include <QWidget>
#include <QBasicTimer>
#include "linetypes.h"

class Overview;

class OverviewWidget : public QWidget
{
  Q_OBJECT

  public:
    OverviewWidget( QWidget* parent = nullptr );

    // Associate the widget with an Overview object.
    void setOverview( Overview* overview ) { overview_ = overview; }

  public slots:
    // Sent when a match at the line passed must be highlighted in
    // the overview
    void highlightLine(LineNumber line );
    void removeHighlight();

  protected:
    void paintEvent( QPaintEvent* paintEvent ) override;
    void mousePressEvent( QMouseEvent* mouseEvent ) override;
    void mouseMoveEvent( QMouseEvent* mouseEvent ) override;
    void timerEvent( QTimerEvent* event ) override;

  signals:
    // Sent when the user click on a line in the Overview.
    void lineClicked( LineNumber line );

  private:
    // Constants
    static const int LINE_MARGIN;
    static const int STEP_DURATION_MS;
    static const int INITIAL_TTL_VALUE;

    Overview* overview_;

    // Highlight:
    // Which line is higlighted, or -1 if none
    int highlightedLine_;
    // Number of step until the highlight become static
    int highlightedTTL_;

    QBasicTimer highlightTimer_;

    void handleMousePress( int position );
};

#endif
