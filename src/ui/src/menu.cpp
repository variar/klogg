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

#include <QAction>
#include <QActionGroup>
#include <QMenu>
#include <QMouseEvent>
#include <QRect>
#include <QTimerEvent>
#include <QToolTip>

#include "active_screen.h"
#include "menu.h"

// It would be nice to only need action, and have action be the parent,
// however implementation needs the parent menu (see showToolTip), and
// since neither action nor parent is guaranteed to die before the other,
// for memory-management purposes the parent will have to be specified
// explicity (probably, the window owning the action and the menu).
MenuActionToolTipBehavior::MenuActionToolTipBehavior( QAction* menuAction, QMenu* menuParent,
                                                      QObject* parent = nullptr )
    : QObject( parent )
    , action( menuAction )
    , parentMenu( menuParent )
    , toolTipDelayMs( 1000 )
    , timerId( 0 )
    , hoverPoint()
{
    connect( action, &QAction::hovered, this, &MenuActionToolTipBehavior::onActionHovered );
}

int MenuActionToolTipBehavior::toolTipDelay()
{
    return toolTipDelayMs;
}

void MenuActionToolTipBehavior::setToolTipDelay( int delayMs )
{
    toolTipDelayMs = delayMs;
}

void MenuActionToolTipBehavior::timerEvent( QTimerEvent* event )
{
    // Not ours, don't touch
    if ( event->timerId() != timerId ) {
        QObject::timerEvent( event );
        return;
    }

    killTimer( timerId ); // interested in a single shot
    timerId = 0;

    // Has the mouse waited unmoved in one location for 'delay' ms?
    const QPoint& mousePos = QCursor::pos( activeScreen( parentMenu ) );
    if ( hoverPoint == mousePos )
        showToolTip( hoverPoint );
}

void MenuActionToolTipBehavior::onActionHovered()
{
    const QPoint& mousePos = QCursor::pos( activeScreen( parentMenu ) );

    // Hover is fired on keyboard focus over action in menu, ignore it
    const QPoint& relativeMousePos = parentMenu->mapFromGlobal( mousePos );
    if ( !parentMenu->actionGeometry( action ).contains( relativeMousePos ) ) {
        if ( timerId != 0 ) { // once timer expires its check will fail anyway
            killTimer( timerId );
            timerId = 0;
        }
        QToolTip::hideText(); // there might be one currently shown
        return;
    }

    // Record location
    hoverPoint = mousePos;

    // Restart timer
    if ( timerId != 0 )
        killTimer( timerId );
    timerId = startTimer( toolTipDelayMs );
}

void MenuActionToolTipBehavior::showToolTip( const QPoint& position )
{
    const QString& toolTip = action->toolTip();
    // Show tooltip until mouse moves at all
    // NOTE: using action->parentWidget() which is the MainWindow,
    // does not work (tooltip is not cleared when upon leaving the
    // region). This is the only reason we need parentMenu here. Just
    // a wild guess: maybe it isn't cleared because it would be
    // cleared on a mouse move over the designated widget, but mouse
    // move doesn't happen over MainWindow, since the mouse is over
    // the menu even when out of the activeRegion.
    QPoint relativePos = parentMenu->mapFromGlobal( position );
    QRect activeRegion( relativePos.x(), relativePos.y(), 1, 1 );
    QToolTip::showText( position, toolTip, parentMenu, activeRegion );
}

HoverMenu::HoverMenu( const QString& title, QWidget* parent )
    : QMenu( title, parent )
    , mouseInMenu_{ false }
{
}

void HoverMenu::mouseMoveEvent( QMouseEvent* ev )
{
    auto nowMousePosition = mouseInMenu( ev->pos() );

    // hide the menu list if the mouse leaves the menu.
    if ( mouseInMenu_ && !nowMousePosition ) {
        this->hide();
    }
    mouseInMenu_ = nowMousePosition;

    QMenu::mouseMoveEvent( ev );
}

void HoverMenu::mouseReleaseEvent( QMouseEvent* ev )
{
    auto* action = this->actionAt( ev->pos() );

    // responds to left-click event only
    if ( action != nullptr && ev->button() == Qt::LeftButton ) {
        action->activate( QAction::ActionEvent::Trigger );
    }
    else {
        QMenu::mouseReleaseEvent( ev );
    }
}
