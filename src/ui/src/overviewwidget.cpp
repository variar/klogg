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

// This file implements OverviewWidget.  This class is responsable for
// managing and painting the matches overview widget.

#include <QMouseEvent>
#include <QPainter>
#include <cassert>

#include "log.h"

#include "overviewwidget.h"

#include "overview.h"

#define HIGHLIGHT_XPM_WIDTH 27
#define HIGHLIGHT_XPM_HEIGHT 9

#define S( x ) #x
#define SX( x ) S( x )

// width height colours char/pixel
// Colours
#define HIGHLIGHT_XPM_LEAD_LINE                                                                    \
    SX( HIGHLIGHT_XPM_WIDTH )                                                                      \
    " " SX( HIGHLIGHT_XPM_HEIGHT ) " 2 1", "  s mask c none", "x c #572F80"

const char* const highlight_xpm[][ 14 ] = {
    {
        HIGHLIGHT_XPM_LEAD_LINE,
        "                           ",
        "                           ",
        "   xxxxxxxxxxxxxxxxxxxxx   ",
        "   xxxxxxxxxxxxxxxxxxxxx   ",
        "   xx                 xx   ",
        "   xx                 xx   ",
        "   xx                 xx   ",
        "   xxxxxxxxxxxxxxxxxxxxx   ",
        "   xxxxxxxxxxxxxxxxxxxxx   ",
        "                           ",
        "                           ",
    },
    {
        HIGHLIGHT_XPM_LEAD_LINE,
        "                           ",
        "  xxxxxxxxxxxxxxxxxxxxxxx  ",
        "  xxxxxxxxxxxxxxxxxxxxxxx  ",
        "  xxxxxxxxxxxxxxxxxxxxxxx  ",
        "  xxx                 xxx  ",
        "  xxx                 xxx  ",
        "  xxx                 xxx  ",
        "  xxxxxxxxxxxxxxxxxxxxxxx  ",
        "  xxxxxxxxxxxxxxxxxxxxxxx  ",
        "  xxxxxxxxxxxxxxxxxxxxxxx  ",
        "                           ",
    },
    {
        HIGHLIGHT_XPM_LEAD_LINE,
        " xxxxxxxxxxxxxxxxxxxxxxxxx ",
        " xxxxxxxxxxxxxxxxxxxxxxxxx ",
        " xxxxxxxxxxxxxxxxxxxxxxxxx ",
        " xxxxxxxxxxxxxxxxxxxxxxxxx ",
        " xxxx                 xxxx ",
        " xxxx                 xxxx ",
        " xxxx                 xxxx ",
        " xxxxxxxxxxxxxxxxxxxxxxxxx ",
        " xxxxxxxxxxxxxxxxxxxxxxxxx ",
        " xxxxxxxxxxxxxxxxxxxxxxxxx ",
        " xxxxxxxxxxxxxxxxxxxxxxxxx ",
    },
    {
        HIGHLIGHT_XPM_LEAD_LINE,
        "                           ",
        "  xxxxxxxxxxxxxxxxxxxxxxx  ",
        "  xxxxxxxxxxxxxxxxxxxxxxx  ",
        "  xxxxxxxxxxxxxxxxxxxxxxx  ",
        "  xxx                 xxx  ",
        "  xxx                 xxx  ",
        "  xxx                 xxx  ",
        "  xxxxxxxxxxxxxxxxxxxxxxx  ",
        "  xxxxxxxxxxxxxxxxxxxxxxx  ",
        "  xxxxxxxxxxxxxxxxxxxxxxx  ",
        "                           ",
    },
    {
        HIGHLIGHT_XPM_LEAD_LINE,
        "                           ",
        "                           ",
        "   xxxxxxxxxxxxxxxxxxxxx   ",
        "   xxxxxxxxxxxxxxxxxxxxx   ",
        "   xx                 xx   ",
        "   xx                 xx   ",
        "   xx                 xx   ",
        "   xxxxxxxxxxxxxxxxxxxxx   ",
        "   xxxxxxxxxxxxxxxxxxxxx   ",
        "                           ",
        "                           ",
    },
    {
        HIGHLIGHT_XPM_LEAD_LINE,
        "                           ",
        "                           ",
        "                           ",
        "    xxxxxxxxxxxxxxxxxxx    ",
        "    x                 x    ",
        "    x                 x    ",
        "    x                 x    ",
        "    xxxxxxxxxxxxxxxxxxx    ",
        "                           ",
        "                           ",
        "                           ",
    },
};

OverviewWidget::OverviewWidget( QWidget* parent )
    : QWidget( parent )
    , highlightTimer_()
{
    setBackgroundRole( QPalette::Window );

    // We should be hidden by default (e.g. for the FilteredView)
    hide();
}

void OverviewWidget::paintEvent( QPaintEvent* /* paintEvent */ )
{
    static const QColor match_color( "red" );
    static const QColor mark_color( "dodgerblue" );

    static const QPixmap highlight_pixmap[] = {
        QPixmap( highlight_xpm[ 0 ] ), QPixmap( highlight_xpm[ 1 ] ), QPixmap( highlight_xpm[ 2 ] ),
        QPixmap( highlight_xpm[ 3 ] ), QPixmap( highlight_xpm[ 4 ] ), QPixmap( highlight_xpm[ 5 ] ),
    };

    // We must be hidden until we have an Overview
    assert( overview_ != nullptr );

    overview_->updateView( static_cast<unsigned>( height() ) );

    {
        QPainter painter( this );

        painter.fillRect( painter.viewport(), painter.background() );

        // The line separating from the main view
        painter.setPen( palette().color( QPalette::Text ) );
        painter.drawLine( 0, 0, 0, height() );

        // The 'match' lines
        painter.setPen( match_color );
        const auto matchLines = *( overview_->getMatchLines() );
        for ( const auto& line : matchLines ) {
            painter.setOpacity( ( 1.0 / Overview::WeightedLine::WEIGHT_STEPS )
                                * ( line.weight() + 1 ) );
            // (allow multiple matches to look 'darker' than a single one.)
            painter.drawLine( 1 + LINE_MARGIN, line.position(), width() - LINE_MARGIN - 1,
                              line.position() );
        }

        // The 'mark' lines
        painter.setPen( mark_color );
        const auto markLines = *( overview_->getMarkLines() );
        for ( const auto& line : markLines ) {
            painter.setOpacity( ( 1.0 / Overview::WeightedLine::WEIGHT_STEPS )
                                * ( line.weight() + 1 ) );
            // (allow multiple matches to look 'darker' than a single one.)
            painter.drawLine( 1 + LINE_MARGIN, line.position(), width() - LINE_MARGIN - 1,
                              line.position() );
        }

        // The 'view' lines
        painter.setOpacity( 1 );
        painter.setPen( palette().color( QPalette::Text ) );
        std::pair<int, int> viewLines = overview_->getViewLines();
        painter.drawLine( 1, viewLines.first, width(), viewLines.first );
        painter.drawLine( 1, viewLines.second, width(), viewLines.second );

        // The highlight
        if ( highlightedLine_ ) {
            /*
            QPen highlight_pen( palette().color(QPalette::Text) );
            highlight_pen.setWidth( 4 - highlightedTTL_ );
            painter.setOpacity( 1 );
            painter.setPen( highlight_pen );
            painter.drawRect( 2, position - 2, width() - 2 - 2, 4 );
            */
            int position = overview_->yFromFileLine( *highlightedLine_ );
            painter.drawPixmap( ( width() - HIGHLIGHT_XPM_WIDTH ) / 2,
                                position - ( HIGHLIGHT_XPM_HEIGHT / 2 ),
                                highlight_pixmap[ INITIAL_TTL_VALUE - highlightedTTL_ ] );
        }
    }
}

void OverviewWidget::mousePressEvent( QMouseEvent* mouseEvent )
{
    if ( mouseEvent->button() == Qt::LeftButton )
        handleMousePress( mouseEvent->y() );
}

void OverviewWidget::mouseMoveEvent( QMouseEvent* mouseEvent )
{
    if ( mouseEvent->buttons().testFlag( Qt::LeftButton ) )
        handleMousePress( mouseEvent->y() );
}

void OverviewWidget::handleMousePress( int position )
{
    const auto line = overview_->fileLineFromY( position );
    LOG_DEBUG << "OverviewWidget::handleMousePress y=" << position << " line=" << line;
    emit lineClicked( line );
}

void OverviewWidget::highlightLine( LineNumber line )
{
    highlightTimer_.stop();

    highlightedLine_ = line;
    highlightedTTL_ = INITIAL_TTL_VALUE;

    update();
    highlightTimer_.start( STEP_DURATION_MS, this );
}

void OverviewWidget::removeHighlight()
{
    highlightTimer_.stop();

    highlightedLine_.reset();
    update();
}

void OverviewWidget::timerEvent( QTimerEvent* event )
{
    if ( event->timerId() == highlightTimer_.timerId() ) {
        LOG_DEBUG << "OverviewWidget::timerEvent";
        if ( highlightedTTL_ > 0 ) {
            --highlightedTTL_;
            update();
        }
        else {
            highlightTimer_.stop();
        }
    }
    else {
        QObject::timerEvent( event );
    }
}
