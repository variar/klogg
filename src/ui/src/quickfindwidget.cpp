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

#include <QToolButton>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <QHBoxLayout>

#include "configuration.h"
#include "qfnotifications.h"

#include "quickfindwidget.h"

const int QuickFindWidget::NOTIFICATION_TIMEOUT = 5000;

const QString QFNotification::REACHED_EOF = "Reached end of file, no occurrence found.";
const QString QFNotification::REACHED_BOF = "Reached beginning of file, no occurrence found.";

QuickFindWidget::QuickFindWidget( QWidget* parent ) : QWidget( parent )
{
    // ui_.setupUi( this );
    // setFocusProxy(ui_.findEdit);
    // setProperty("topBorder", true);
    auto *layout = new QHBoxLayout( this );

    layout->setContentsMargins( 6, 0, 6, 6 );

    closeButton_ = setupToolButton(
            QLatin1String(""), QLatin1String( ":/images/darkclosebutton.png" ) );
    layout->addWidget( closeButton_ );

    editQuickFind_ = new QLineEdit( this );
    // FIXME: set MinimumSize might be to constraining
    editQuickFind_->setMinimumSize( QSize( 150, 0 ) );
    layout->addWidget( editQuickFind_ );

    ignoreCaseCheck_ = new QCheckBox( "Ignore &case" );
    layout->addWidget( ignoreCaseCheck_ );

    previousButton_ = setupToolButton( QLatin1String("Previous"),
            QLatin1String( ":/images/arrowup.png" ) );
    layout->addWidget( previousButton_ );

    nextButton_ = setupToolButton( QLatin1String("Next"),
            QLatin1String( ":/images/arrowdown.png" ) );
    layout->addWidget( nextButton_ );

    notificationText_ = new QLabel( "" );
    // FIXME: set MinimumSize might be too constraining
    int width = QFNotification::maxWidth( notificationText_ );
    notificationText_->setMinimumSize( width, 0 );
    layout->addWidget( notificationText_ );

    setMinimumWidth( minimumSizeHint().width() );

    // Behaviour
    connect( closeButton_, SIGNAL( clicked() ), SLOT( closeHandler() ) );
    connect( editQuickFind_, SIGNAL( textEdited( QString ) ),
             this, SLOT( textChanged() ) );
    connect( ignoreCaseCheck_, SIGNAL( stateChanged( int ) ),
             this, SLOT( textChanged() ) );
    /*
    connect( editQuickFind_. SIGNAL( textChanged( QString ) ), this,
            SLOT( updateButtons() ) );
    */

    connect( editQuickFind_, SIGNAL( returnPressed() ),
             this, SLOT( returnHandler() ) );
    connect( previousButton_, SIGNAL( clicked() ),
            this, SLOT( doSearchBackward() ) );
    connect( nextButton_, SIGNAL( clicked() ),
            this, SLOT( doSearchForward() ) );

    // Notification timer:
    notificationTimer_ = new QTimer( this );
    notificationTimer_->setSingleShot( true );
    connect( notificationTimer_, SIGNAL( timeout() ),
            this, SLOT( notificationTimeout() ) );
}

void QuickFindWidget::userActivate()
{
    userRequested_ = true;
    QWidget::show();
    editQuickFind_->setFocus( Qt::ShortcutFocusReason );
    editQuickFind_->selectAll();
}

//
// SLOTS
//

void QuickFindWidget::changeDisplayedPattern( const QString& newPattern )
{
    editQuickFind_->setText( newPattern );
    editQuickFind_->setCursorPosition( patternCursorPosition_ );
}

void QuickFindWidget::notify( const QFNotification& message )
{
    LOG(logDEBUG) << "QuickFindWidget::notify()";

    notificationText_->setText( message.message() );
    QWidget::show();
    notificationTimer_->start( NOTIFICATION_TIMEOUT );
}

void QuickFindWidget::clearNotification()
{
    LOG(logDEBUG) << "QuickFindWidget::clearNotification()";

    notificationText_->setText( "" );
}

// User clicks forward arrow
void QuickFindWidget::doSearchForward()
{
    LOG(logDEBUG) << "QuickFindWidget::doSearchForward()";

    // The user has clicked on a button, so we assume she wants
    // the widget to stay visible.
    userRequested_ = true;

    emit patternConfirmed( editQuickFind_->text(), isIgnoreCase() );
    emit searchForward();
}

// User clicks backward arrow
void QuickFindWidget::doSearchBackward()
{
    LOG(logDEBUG) << "QuickFindWidget::doSearchBackward()";

    // The user has clicked on a button, so we assume she wants
    // the widget to stay visible.
    userRequested_ = true;

    emit patternConfirmed( editQuickFind_->text(), isIgnoreCase() );
    emit searchBackward();
}

// Close and search when the user presses Return
void QuickFindWidget::returnHandler()
{
    emit patternConfirmed( editQuickFind_->text(), isIgnoreCase() );
    // Close the widget
    userRequested_ = false;
    this->hide();
    emit close();
}

// Close and reset flag when the user clicks 'close'
void QuickFindWidget::closeHandler()
{
    userRequested_ = false;
    this->hide();
    emit close();
    emit cancelSearch();
}

void QuickFindWidget::notificationTimeout()
{
    // We close the widget if the user hasn't explicitely requested it.
    if ( userRequested_ == false )
        this->hide();
}

void QuickFindWidget::textChanged()
{
    patternCursorPosition_ = editQuickFind_->cursorPosition();
    emit patternUpdated( editQuickFind_->text(), isIgnoreCase() );
}

//
// Private functions
//
QToolButton* QuickFindWidget::setupToolButton(
        const QString &text, const QString &icon)
{
    auto *toolButton = new QToolButton(this);

    toolButton->setAutoRaise(true);
    toolButton->setIcon(QIcon(icon));

    if(text.length()>0) {
        toolButton->setText(text);
        toolButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    } else {
        toolButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    }

    return toolButton;
}

bool QuickFindWidget::isIgnoreCase() const
{
    return ( ignoreCaseCheck_->checkState() == Qt::Checked );
}
