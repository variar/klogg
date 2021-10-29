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

#include "log.h"

#include "infoline.h"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QMenu>
#include <QPainter>

InfoLine::InfoLine()
{
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    setTextInteractionFlags( Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard );
}

void InfoLine::displayGauge( int completion )
{
    if ( !origPalette_ ) {
        origPalette_ = palette();
    }

    int changeoverX = width() * completion / 100;

    // Create a gradient for the progress bar
    QLinearGradient linearGrad( changeoverX - 1, 0, changeoverX + 1, 0 );
    linearGrad.setColorAt( 0, origPalette_->color( QPalette::Highlight ) );
    linearGrad.setColorAt( 1, origPalette_->color( QPalette::Window ) );

    // Apply the gradient to the current palette (background)
    QPalette newPalette = *origPalette_;
    newPalette.setBrush( backgroundRole(), QBrush( linearGrad ) );
    setPalette( newPalette );
}

void InfoLine::hideGauge()
{
    if ( origPalette_ ) {
        setPalette( *origPalette_ );
    }
    origPalette_.reset();
}

// Custom painter: draw the background then call QLabel's painter
void InfoLine::paintEvent( QPaintEvent* paintEvent )
{
    // Fill the widget background
    {
        QPainter painter( this );
        painter.fillRect( 0, 0, this->width(), this->height(),
                          palette().brush( backgroundRole() ) );
    }

    // Call the parent's painter
    QLabel::paintEvent( paintEvent );
}

void InfoLine::contextMenuEvent( QContextMenuEvent* event )
{
    QMenu menu( this );

    auto copySelection = menu.addAction( "Copy" );
    menu.addSeparator();
    auto selectAll = menu.addAction( "Select all" );

    copySelection->setEnabled( this->hasSelectedText() );
    connect( copySelection, &QAction::triggered,
             [this]( auto ) { QApplication::clipboard()->setText( this->selectedText() ); } );

    connect( selectAll, &QAction::triggered,
             [this]( auto ) { setSelection( 0, static_cast<int>(this->text().length() )); } );

    menu.exec( event->globalPos() );
}
