/*
 * Copyright (C) 2009, 2010, 2011, 2012, 2013, 2015 Nicolas Bonnefon
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

// This file implements the AbstractLogView base class.
// Most of the actual drawing and event management common to the two views
// is implemented in this class.  The class only calls protected virtual
// functions when view specific behaviour is desired, using the template
// pattern.

#include <cassert>
#include <iostream>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QFileDialog>
#include <QFontMetrics>
#include <QGestureEvent>
#include <QMenu>
#include <QPaintEvent>
#include <QPainter>
#include <QProgressDialog>
#include <QRect>
#include <QScrollBar>
#include <QtConcurrent>
#include <QtCore>

#include "log.h"

#include "configuration.h"
#include "highlighterset.h"
#include "logmainview.h"
#include "overview.h"
#include "quickfind.h"
#include "quickfindpattern.h"

namespace {
int mapPullToFollowLength( int length );
}

namespace {

int countDigits( quint64 n )
{
    if ( n == 0 )
        return 1;

    // We must force the compiler to not store intermediate results
    // in registers because this causes incorrect result on some
    // systems under optimizations level >0. For the skeptical:
    //
    // #include <math.h>
    // #include <stdlib.h>
    // int main(int argc, char **argv) {
    //     (void)argc;
    //     long long int n = atoll(argv[1]);
    //     return floor( log( n ) / log( 10 ) + 1 );
    // }
    //
    // This is on Thinkpad T60 (Genuine Intel(R) CPU T2300).
    // $ g++ --version
    // g++ (Ubuntu/Linaro 4.6.3-1ubuntu5) 4.6.3
    // $ g++ -O0 -Wall -W -o math math.cpp -lm; ./math 10; echo $?
    // 2
    // $ g++ -O1 -Wall -W -o math math.cpp -lm; ./math 10; echo $?
    // 1
    //
    // A fix is to (1) explicitly place intermediate results in
    // variables *and* (2) [A] mark them as 'volatile', or [B] pass
    // -ffloat-store to g++ (note that approach [A] is more portable).

    volatile qreal ln_n = qLn( n );
    volatile qreal ln_10 = qLn( 10 );
    volatile qreal lg_n = ln_n / ln_10;
    volatile qreal lg_n_1 = lg_n + 1;
    volatile qreal fl_lg_n_1 = qFloor( lg_n_1 );

    return fl_lg_n_1;
}

} // namespace

LineChunk::LineChunk( int first_col, int last_col, ChunkType type )
{
    // LOG(logDEBUG) << "new LineChunk: " << first_col << " " << last_col;

    start_ = first_col;
    end_ = last_col;
    type_ = type;
}

std::vector<LineChunk> LineChunk::select( int sel_start, int sel_end ) const
{
    std::vector<LineChunk> list;

    if ( ( sel_start < start_ ) && ( sel_end < start_ ) ) {
        // Selection BEFORE this chunk: no change
        list.emplace_back( *this );
    }
    else if ( sel_start > end_ ) {
        // Selection AFTER this chunk: no change
        list.emplace_back( *this );
    }
    else /* if ( ( sel_start >= start_ ) && ( sel_end <= end_ ) ) */
    {
        // We only want to consider what's inside THIS chunk
        sel_start = qMax( sel_start, start_ );
        sel_end = qMin( sel_end, end_ );

        if ( sel_start > start_ ) {
            list.emplace_back( start_, sel_start - 1, type_ );
        }

        list.emplace_back( sel_start, sel_end, Selected );

        if ( sel_end < end_ ) {
            list.emplace_back( sel_end + 1, end_, type_ );
        }
    }

    return list;
}

inline void LineDrawer::addChunk( int first_col, int last_col, QColor fore, QColor back )
{
    if ( first_col < 0 )
        first_col = 0;
    int length = last_col - first_col + 1;
    if ( length > 0 ) {
        list.emplace_back( first_col, length, fore, back );
    }
}

inline void LineDrawer::addChunk( const LineChunk& chunk, QColor fore, QColor back )
{
    int first_col = chunk.start();
    int last_col = chunk.end();

    addChunk( first_col, last_col, fore, back );
}

inline void LineDrawer::draw( QPainter& painter, int initialXPos, int initialYPos, int line_width,
                              const QString& line, int leftExtraBackgroundPx )
{
    QFontMetrics fm = painter.fontMetrics();
    const int fontHeight = fm.height();
    const int fontAscent = fm.ascent();
#if ( QT_VERSION >= QT_VERSION_CHECK( 5, 11, 0 ) )
    const int fontWidth = fm.horizontalAdvance( QChar( 'a' ) );
#else
    const int fontWidth = fm.width( QChar( 'a' ) );
#endif

    int xPos = initialXPos;
    int yPos = initialYPos;

    for ( const Chunk& chunk : list ) {
        // Draw each chunk
        // LOG(logDEBUG) << "Chunk: " << chunk.start() << " " << chunk.length();
        QString cutline = line.mid( chunk.start(), chunk.length() );
        const int chunk_width = cutline.length() * fontWidth;
        if ( xPos == initialXPos ) {
            // First chunk, we extend the left background a bit,
            // it looks prettier.
            painter.fillRect( xPos - leftExtraBackgroundPx, yPos,
                              chunk_width + leftExtraBackgroundPx, fontHeight, chunk.backColor() );
        }
        else {
            // other chunks...
            painter.fillRect( xPos, yPos, chunk_width, fontHeight, chunk.backColor() );
        }
        painter.setPen( chunk.foreColor() );
        painter.drawText( xPos, yPos + fontAscent, cutline );
        xPos += chunk_width;
    }

    // Draw the empty block at the end of the line
    int blank_width = line_width - xPos;

    if ( blank_width > 0 )
        painter.fillRect( xPos, yPos, blank_width, fontHeight, backColor_ );
}

const int DigitsBuffer::timeout_ = 2000;

DigitsBuffer::DigitsBuffer()
    : QObject()
{
}

void DigitsBuffer::reset()
{
    LOG( logDEBUG ) << "DigitsBuffer::reset()";

    timer_.stop();
    digits_.clear();
}

void DigitsBuffer::add( char character )
{
    LOG( logDEBUG ) << "DigitsBuffer::add()";

    digits_.append( QChar( character ) );
    timer_.start( timeout_, this );
}

int DigitsBuffer::content()
{
    int result = digits_.toInt();
    reset();

    return result;
}

bool DigitsBuffer::isEmpty() const
{
    return digits_.isEmpty();
}

void DigitsBuffer::timerEvent( QTimerEvent* event )
{
    if ( event->timerId() == timer_.timerId() ) {
        reset();
    }
    else {
        QObject::timerEvent( event );
    }
}

AbstractLogView::AbstractLogView( const AbstractLogData* newLogData,
                                  const QuickFindPattern* const quickFindPattern, QWidget* parent )
    : QAbstractScrollArea( parent )
    , followElasticHook_( HOOK_THRESHOLD )
    , lineNumbersVisible_( false )
    , selectionStartPos_()
    , selectionCurrentEndPos_()
    , autoScrollTimer_()
    , selection_()
    , searchStart_()
    , quickFindPattern_( quickFindPattern )
    , quickFind_( new QuickFind( *newLogData ) )
{
    logData = newLogData;
    searchEnd_ = LineNumber( logData->getNbLine().get() );

    followMode_ = false;

    selectionStarted_ = false;
    markingClickInitiated_ = false;

    firstLine = 0_lnum;
    lastLineAligned = false;
    firstCol = 0;

    overview_ = nullptr;
    overviewWidget_ = nullptr;

    // Display
    leftMarginPx_ = 0;

    // Fonts (sensible default for overview widget)
    charWidth_ = 1;
    charHeight_ = 10;

    // Create the viewport QWidget
    setViewport( nullptr );

    // Hovering
    setMouseTracking( true );
    lastHoveredLine_ = {};

    // Init the popup menu
    createMenu();

    // Signals
    connect( quickFindPattern_, SIGNAL( patternUpdated() ), this, SLOT( handlePatternUpdated() ) );
    connect( quickFind_, SIGNAL( notify( const QFNotification& ) ), this,
             SIGNAL( notifyQuickFind( const QFNotification& ) ) );
    connect( quickFind_, SIGNAL( clearNotification() ), this,
             SIGNAL( clearQuickFindNotification() ) );

    connect( quickFind_, &QuickFind::searchDone, this, &AbstractLogView::setQuickFindResult,
             Qt::QueuedConnection );

    connect( &followElasticHook_, SIGNAL( lengthChanged() ), this, SLOT( repaint() ) );
    connect( &followElasticHook_, SIGNAL( hooked( bool ) ), this,
             SIGNAL( followModeChanged( bool ) ) );
}

AbstractLogView::~AbstractLogView()
{
    quickFind_->stopSearch();
}

//
// Received events
//

void AbstractLogView::changeEvent( QEvent* changeEvent )
{
    QAbstractScrollArea::changeEvent( changeEvent );

    // Stop the timer if the widget becomes inactive
    if ( changeEvent->type() == QEvent::ActivationChange ) {
        if ( !isActiveWindow() )
            autoScrollTimer_.stop();
    }
    viewport()->update();
}

void AbstractLogView::mousePressEvent( QMouseEvent* mouseEvent )
{
    const auto& config = Configuration::get();

    if ( mouseEvent->button() == Qt::LeftButton ) {
        const auto line = convertCoordToLine( mouseEvent->y() );

        if ( line.has_value() && mouseEvent->modifiers() & Qt::ShiftModifier ) {
            selection_.selectRangeFromPrevious( *line );
            emit updateLineNumber( *line );
            update();
        }
        else if ( line.has_value() ) {
            if ( mouseEvent->x() < bulletZoneWidthPx_ ) {
                // Mark a line if it is clicked in the left margin
                // (only if click and release in the same area)
                markingClickInitiated_ = true;
                markingClickLine_ = line;
            }
            else {
                // Select the line, and start a selection
                if ( *line < logData->getNbLine() ) {
                    selection_.selectLine( *line );
                    emit updateLineNumber( *line );
                    emit newSelection( *line );
                }

                // Remember the click in case we're starting a selection
                selectionStarted_ = true;
                selectionStartPos_ = convertCoordToFilePos( mouseEvent->pos() );
                selectionCurrentEndPos_ = selectionStartPos_;
            }
        }

        // Invalidate our cache
        textAreaCache_.invalid_ = true;
    }
    else if ( mouseEvent->button() == Qt::RightButton ) {
        // Prepare the popup depending on selection type
        if ( selection_.isSingleLine() ) {
            copyAction_->setText( "&Copy this line" );

            setSearchStartAction_->setEnabled( true );
            setSearchEndAction_->setEnabled( true );
        }
        else {
            copyAction_->setText( "&Copy" );
            copyAction_->setStatusTip( tr( "Copy the selection" ) );

            setSearchStartAction_->setEnabled( false );
            setSearchEndAction_->setEnabled( false );
        }

        if ( selection_.isPortion() ) {
            findNextAction_->setEnabled( true );
            findPreviousAction_->setEnabled( true );
            addToSearchAction_->setEnabled( true );
        }
        else {
            findNextAction_->setEnabled( false );
            findPreviousAction_->setEnabled( false );
            addToSearchAction_->setEnabled( false );
        }

        // "Add to search" only makes sense in regexp mode
        if ( config.mainRegexpType() != ExtendedRegexp )
            addToSearchAction_->setEnabled( false );

        // Display the popup (blocking)
        popupMenu_->exec( QCursor::pos() );
    }

    emit activity();
}

void AbstractLogView::mouseMoveEvent( QMouseEvent* mouseEvent )
{
    // Selection implementation
    if ( selectionStarted_ ) {
        // Invalidate our cache
        textAreaCache_.invalid_ = true;

        QPoint thisEndPos = convertCoordToFilePos( mouseEvent->pos() );
        if ( thisEndPos != selectionCurrentEndPos_ ) {
            const auto lineNumber = LineNumber( thisEndPos.y() );
            // Are we on a different line?
            if ( selectionStartPos_.y() != thisEndPos.y() ) {
                if ( thisEndPos.y() != selectionCurrentEndPos_.y() ) {
                    // This is a 'range' selection
                    selection_.selectRange( LineNumber( selectionStartPos_.y() ), lineNumber );
                    emit updateLineNumber( lineNumber );
                    update();
                }
            }
            // So we are on the same line. Are we moving horizontaly?
            else if ( thisEndPos.x() != selectionCurrentEndPos_.x() ) {
                // This is a 'portion' selection
                selection_.selectPortion( lineNumber, selectionStartPos_.x(), thisEndPos.x() );
                update();
            }
            // On the same line, and moving vertically then
            else {
                // This is a 'line' selection
                selection_.selectLine( lineNumber );
                emit updateLineNumber( lineNumber );
                update();
            }
            selectionCurrentEndPos_ = thisEndPos;

            // Do we need to scroll while extending the selection?
            QRect visible = viewport()->rect();
            if ( visible.contains( mouseEvent->pos() ) )
                autoScrollTimer_.stop();
            else if ( !autoScrollTimer_.isActive() )
                autoScrollTimer_.start( 100, this );
        }
    }
    else {
        considerMouseHovering( mouseEvent->x(), mouseEvent->y() );
    }
}

void AbstractLogView::mouseReleaseEvent( QMouseEvent* mouseEvent )
{
    if ( markingClickInitiated_ ) {
        markingClickInitiated_ = false;
        const auto line = convertCoordToLine( mouseEvent->y() );
        if ( line.has_value() && line == markingClickLine_ ) {
            // Invalidate our cache
            textAreaCache_.invalid_ = true;

            emit markLine( *line );
        }
    }
    else {
        selectionStarted_ = false;
        if ( autoScrollTimer_.isActive() )
            autoScrollTimer_.stop();
        updateGlobalSelection();
    }
}

void AbstractLogView::mouseDoubleClickEvent( QMouseEvent* mouseEvent )
{
    if ( mouseEvent->button() == Qt::LeftButton ) {
        // Invalidate our cache
        textAreaCache_.invalid_ = true;

        const QPoint pos = convertCoordToFilePos( mouseEvent->pos() );
        selectWordAtPosition( pos );
    }

    emit activity();
}

void AbstractLogView::timerEvent( QTimerEvent* timerEvent )
{
    if ( timerEvent->timerId() == autoScrollTimer_.timerId() ) {
        QRect visible = viewport()->rect();
        const QPoint globalPos = QCursor::pos();
        const QPoint pos = viewport()->mapFromGlobal( globalPos );
        QMouseEvent ev( QEvent::MouseMove, pos, globalPos, Qt::LeftButton, Qt::LeftButton,
                        Qt::NoModifier );
        mouseMoveEvent( &ev );
        int deltaX = qMax( pos.x() - visible.left(), visible.right() - pos.x() ) - visible.width();
        int deltaY = qMax( pos.y() - visible.top(), visible.bottom() - pos.y() ) - visible.height();
        int delta = qMax( deltaX, deltaY );

        if ( delta >= 0 ) {
            if ( delta < 7 )
                delta = 7;
            int timeout = 4900 / ( delta * delta );
            autoScrollTimer_.start( timeout, this );

            if ( deltaX > 0 )
                horizontalScrollBar()->triggerAction( pos.x() < visible.center().x()
                                                          ? QAbstractSlider::SliderSingleStepSub
                                                          : QAbstractSlider::SliderSingleStepAdd );

            if ( deltaY > 0 )
                verticalScrollBar()->triggerAction( pos.y() < visible.center().y()
                                                        ? QAbstractSlider::SliderSingleStepSub
                                                        : QAbstractSlider::SliderSingleStepAdd );
        }
    }
    QAbstractScrollArea::timerEvent( timerEvent );
}

void AbstractLogView::moveSelectionUp()
{
    int delta = qMin( -1, -digitsBuffer_.content() );
    disableFollow();
    moveSelection( delta );
}

void AbstractLogView::moveSelectionDown()
{
    int delta = qMax( 1, digitsBuffer_.content() );
    disableFollow();
    moveSelection( delta );
}

void AbstractLogView::keyPressEvent( QKeyEvent* keyEvent )
{
    LOG( logDEBUG4 ) << "keyPressEvent received";

    const auto controlModifier = keyEvent->modifiers().testFlag( Qt::ControlModifier );
    const auto shiftModifier = keyEvent->modifiers().testFlag( Qt::ShiftModifier );
    const auto noModifier = keyEvent->modifiers() == Qt::NoModifier;

    const auto jumpToBottomLine = [this]() {
        disableFollow();
        const auto line = LineNumber( logData->getNbLine().get() ) - 1_lcount;
        selection_.selectLine( line );
        emit updateLineNumber( line );
        emit newSelection( line );
        jumpToBottom();
    };

    if ( keyEvent->key() == Qt::Key_Up && noModifier )
        moveSelectionUp();
    else if ( keyEvent->key() == Qt::Key_Down && noModifier )
        moveSelectionDown();
    else if ( keyEvent->key() == Qt::Key_Left && noModifier )
        horizontalScrollBar()->triggerAction( QScrollBar::SliderPageStepSub );
    else if ( keyEvent->key() == Qt::Key_Right && noModifier )
        horizontalScrollBar()->triggerAction( QScrollBar::SliderPageStepAdd );
    else if ( keyEvent->key() == Qt::Key_Home && !controlModifier )
        jumpToStartOfLine();
    else if ( keyEvent->key() == Qt::Key_End && !controlModifier )
        jumpToRightOfScreen();
    else if ( ( keyEvent->key() == Qt::Key_PageDown && controlModifier )
              || ( keyEvent->key() == Qt::Key_End && controlModifier ) ) {
        jumpToBottomLine(); // Same as G
    }
    else if ( ( keyEvent->key() == Qt::Key_PageUp && controlModifier )
              || ( keyEvent->key() == Qt::Key_Home && controlModifier ) )
        selectAndDisplayLine( 0_lnum );
    else if ( keyEvent->key() == Qt::Key_F3 && !shiftModifier )
        emit searchNext(); // duplicate of 'n' action.
    else if ( keyEvent->key() == Qt::Key_F3 && shiftModifier )
        emit searchPrevious(); // duplicate of 'N' action.
    else if ( keyEvent->key() == Qt::Key_Space && noModifier )
        emit exitView();
    else {
        const auto text = keyEvent->text();

        if ( keyEvent->modifiers() == Qt::NoModifier && text.count() == 1 ) {
            const auto character = text.at( 0 ).toLatin1();
            if ( ( ( character > '0' ) && ( character <= '9' ) )
                 || ( !digitsBuffer_.isEmpty() && character == '0' ) ) {
                // Adds the digit to the timed buffer
                digitsBuffer_.add( character );
            }
        }
        else {
            switch ( keyEvent->key() ) {
            case Qt::Key_J:
                moveSelectionDown();
                break;
            case Qt::Key_K:
                moveSelectionUp();
                break;
            case Qt::Key_H:
                horizontalScrollBar()->triggerAction( QScrollBar::SliderSingleStepSub );
                break;
            case Qt::Key_L:
                horizontalScrollBar()->triggerAction( QScrollBar::SliderSingleStepAdd );
                break;
            case Qt::Key_0:
            case Qt::Key_AsciiCircum:
                jumpToStartOfLine();
                break;
            case Qt::Key_Dollar:
                jumpToEndOfLine();
                break;
            case Qt::Key_Asterisk:
            case Qt::Key_Period:
                // Use the selected 'word' and search forward
                findNextSelected();
                break;
            case Qt::Key_Slash:
            case Qt::Key_Comma:
                // Use the selected 'word' and search backward
                findPreviousSelected();
                break;
            case Qt::Key_M: {
                auto line = selection_.selectedLine();
                if ( line.has_value() )
                    emit markLine( *line );
                break;
            }
            case Qt::Key_G:
                if ( controlModifier ) {
                    if ( shiftModifier ) {
                        emit searchPrevious();
                    }
                    else {
                        emit searchNext();
                    }
                    break;
                }
                if ( shiftModifier ) {
                    jumpToBottomLine();
                }
                else {
                    LineNumber::UnderlyingType newLine = qMax( 0, digitsBuffer_.content() - 1 );
                    if ( newLine >= logData->getNbLine().get() )
                        newLine = logData->getNbLine().get() - 1;
                    selectAndDisplayLine( LineNumber( newLine ) );
                }
                break;
            case Qt::Key_N:
                if ( shiftModifier ) {
                    emit searchPrevious();
                }
                else {
                    emit searchNext();
                }
                break;
            default:
                keyEvent->ignore();
            }
        }
    }

    if ( keyEvent->isAccepted() ) {
        emit activity();
    }
    else {
        // Only pass bare keys to the superclass this is so that
        // shortcuts such as Ctrl+Alt+Arrow are handled by the parent.
        LOG( logDEBUG ) << std::hex << keyEvent->modifiers();
        if ( keyEvent->modifiers() == Qt::NoModifier
             || keyEvent->modifiers() == Qt::KeypadModifier ) {
            QAbstractScrollArea::keyPressEvent( keyEvent );
        }
    }
}

void AbstractLogView::wheelEvent( QWheelEvent* wheelEvent )
{
    emit activity();

    // LOG(logDEBUG) << "wheelEvent";

    // This is to handle the case where follow mode is on, but the user
    // has moved using the scroll bar. We take them back to the bottom.
    if ( followMode_ )
        jumpToBottom();

    if ( verticalScrollBar()->value() == verticalScrollBar()->maximum() ) {
        // First see if we need to block the elastic (on Mac)
        if ( wheelEvent->phase() == Qt::ScrollBegin )
            followElasticHook_.hold();
        else if ( wheelEvent->phase() == Qt::ScrollEnd )
            followElasticHook_.release();

        int y_delta = 0;
        auto pixel_delta = wheelEvent->pixelDelta();

        if ( pixel_delta.isNull() ) {
            y_delta = wheelEvent->angleDelta().y() / 0.7;
        }
        else {
            y_delta = pixel_delta.y();
        }

        // LOG(logDEBUG) << "Elastic " << y_delta;
        followElasticHook_.move( -y_delta );
    }

    // LOG(logDEBUG) << "Length = " << followElasticHook_.length();
    if ( followElasticHook_.length() == 0 && !followElasticHook_.isHooked() ) {
        QAbstractScrollArea::wheelEvent( wheelEvent );
    }
}

void AbstractLogView::resizeEvent( QResizeEvent* )
{
    if ( logData == nullptr )
        return;

    LOG( logDEBUG1 ) << "resizeEvent received";

    updateDisplaySize();
}

bool AbstractLogView::event( QEvent* e )
{
    LOG( logDEBUG4 ) << "Event! Type: " << e->type();

    // Make sure we ignore the gesture events as
    // they seem to be accepted by default.
    if ( e->type() == QEvent::Gesture ) {
        auto gesture_event = dynamic_cast<QGestureEvent*>( e );
        if ( gesture_event ) {
            const auto gestures = gesture_event->gestures();
            for ( QGesture* gesture : gestures ) {
                LOG( logDEBUG4 ) << "Gesture: " << gesture->gestureType();
                gesture_event->ignore( gesture );
            }

            // Ensure the event is sent up to parents who might care
            return false;
        }
    }

    return QAbstractScrollArea::event( e );
}

void AbstractLogView::scrollContentsBy( int dx, int dy )
{
    LOG( logDEBUG ) << "scrollContentsBy received " << dy << "position "
                    << verticalScrollBar()->value();

    const auto last_top_line = ( logData->getNbLine() - getNbVisibleLines() );
    const auto scrollPosition
        = static_cast<LinesCount::UnderlyingType>( verticalScrollBar()->value() );
    if ( ( last_top_line.get() > 0 ) && scrollPosition > last_top_line.get() ) {
        // The user is going further than the last line, we need to lock the last line at the bottom
        LOG( logDEBUG ) << "scrollContentsBy beyond!";
        firstLine = LineNumber( scrollPosition );
        lastLineAligned = true;
    }
    else {
        firstLine = LineNumber( scrollPosition );
        lastLineAligned = false;
    }

    firstCol = ( firstCol - dx ) > 0 ? firstCol - dx : 0;
    const auto last_line = firstLine + getNbVisibleLines();

    // Update the overview if we have one
    if ( overview_ != nullptr )
        overview_->updateCurrentPosition( firstLine, last_line );

    // Are we hovering over a new line?
    const QPoint mouse_pos = mapFromGlobal( QCursor::pos() );
    considerMouseHovering( mouse_pos.x(), mouse_pos.y() );

    // Redraw
    update();
}

void AbstractLogView::paintEvent( QPaintEvent* paintEvent )
{
    const QRect invalidRect = paintEvent->rect();
    if ( ( invalidRect.isEmpty() ) || ( logData == nullptr ) )
        return;

    LOG( logDEBUG4 ) << "paintEvent received, firstLine=" << firstLine
                     << " lastLineAligned=" << lastLineAligned
                     << " rect: " << invalidRect.topLeft().x() << ", " << invalidRect.topLeft().y()
                     << ", " << invalidRect.bottomRight().x() << ", "
                     << invalidRect.bottomRight().y();

#ifdef GLOGG_PERF_MEASURE_FPS
    static uint32_t maxline = logData->getNbLine();
    if ( !perfCounter_.addEvent() && logData->getNbLine() > maxline ) {
        LOG( logWARNING ) << "Redraw per second: " << perfCounter_.readAndReset()
                          << " lines: " << logData->getNbLine();
        perfCounter_.addEvent();
        maxline = logData->getNbLine();
    }
#endif

    auto start = std::chrono::system_clock::now();

    // Can we use our cache?
    int32_t delta_y = textAreaCache_.first_line_.get() - firstLine.get();

    if ( textAreaCache_.invalid_ || ( textAreaCache_.first_column_ != firstCol ) ) {
        // Force a full redraw
        delta_y = INT32_MAX;
    }

    if ( delta_y != 0 ) {
        // Full or partial redraw
        drawTextArea( &textAreaCache_.pixmap_, delta_y );

        textAreaCache_.invalid_ = false;
        textAreaCache_.first_line_ = firstLine;
        textAreaCache_.first_column_ = firstCol;

        LOG( logDEBUG1 ) << "End of writing "
                         << std::chrono::duration_cast<std::chrono::microseconds>(
                                std::chrono::system_clock::now() - start )
                                .count();
    }
    else {
        // Use the cache as is: nothing to do!
    }

    // Height including the potentially invisible last line
    const int whole_height = getNbVisibleLines().get() * charHeight_;
    // Height in pixels of the "pull to follow" bottom bar.
    int pullToFollowHeight
        = mapPullToFollowLength( followElasticHook_.length() )
          + ( followElasticHook_.isHooked()
                  ? ( whole_height - viewport()->height() ) + PULL_TO_FOLLOW_HOOKED_HEIGHT
                  : 0 );

    if ( pullToFollowHeight && ( pullToFollowCache_.nb_columns_ != getNbVisibleCols() ) ) {
        LOG( logDEBUG1 ) << "Drawing pull to follow bar";
        pullToFollowCache_.pixmap_
            = drawPullToFollowBar( viewport()->width(), viewport()->devicePixelRatio() );
        pullToFollowCache_.nb_columns_ = getNbVisibleCols();
    }

    QPainter devicePainter( viewport() );
    int drawingTopPosition = -pullToFollowHeight;
    int drawingPullToFollowTopPosition = drawingTopPosition + whole_height;

    // This is to cover the special case where there is less than a screenful
    // worth of data, we want to see the document from the top, rather than
    // pushing the first couple of lines above the viewport.
    if ( followElasticHook_.isHooked() && ( logData->getNbLine() < getNbVisibleLines() ) ) {
        drawingTopOffset_ = 0;
        drawingTopPosition
            += ( whole_height - viewport()->height() ) + PULL_TO_FOLLOW_HOOKED_HEIGHT;
        drawingPullToFollowTopPosition
            = drawingTopPosition + viewport()->height() - PULL_TO_FOLLOW_HOOKED_HEIGHT;
    }
    // This is the case where the user is on the 'extra' slot at the end
    // and is aligned on the last line (but no elastic shown)
    else if ( lastLineAligned && !followElasticHook_.isHooked() ) {
        drawingTopOffset_ = -( whole_height - viewport()->height() );
        drawingTopPosition += drawingTopOffset_;
        drawingPullToFollowTopPosition = drawingTopPosition + whole_height;
    }
    else {
        drawingTopOffset_ = -pullToFollowHeight;
    }

    devicePainter.drawPixmap( 0, drawingTopPosition, textAreaCache_.pixmap_ );

    // Draw the "pull to follow" zone if needed
    if ( pullToFollowHeight ) {
        devicePainter.drawPixmap( 0, drawingPullToFollowTopPosition, pullToFollowCache_.pixmap_ );
    }

    LOG( logDEBUG1 ) << "End of repaint "
                     << std::chrono::duration_cast<std::chrono::microseconds>(
                            std::chrono::system_clock::now() - start )
                            .count();
}

// These two functions are virtual and this implementation is clearly
// only valid for a non-filtered display.
// We count on the 'filtered' derived classes to override them.
LineNumber AbstractLogView::displayLineNumber( LineNumber lineNumber ) const
{
    return lineNumber + 1_lcount; // show a 1-based index
}

LineNumber AbstractLogView::lineIndex( LineNumber lineNumber ) const
{
    return lineNumber;
}

LineNumber AbstractLogView::maxDisplayLineNumber() const
{
    return LineNumber( logData->getNbLine().get() );
}

void AbstractLogView::setOverview( Overview* overview, OverviewWidget* overview_widget )
{
    overview_ = overview;
    overviewWidget_ = overview_widget;

    if ( overviewWidget_ ) {
        connect( overviewWidget_, &OverviewWidget::lineClicked, this,
                 &AbstractLogView::jumpToLine );
    }
    refreshOverview();
}

LineNumber AbstractLogView::getViewPosition() const
{
    LineNumber line;

    const auto selectedLine = selection_.selectedLine();
    if ( selectedLine.has_value() ) {
        line = *selectedLine;
    }
    else {
        // Middle of the view
        line = firstLine + LinesCount( getNbVisibleLines().get() / 2 );
    }

    return line;
}

void AbstractLogView::searchUsingFunction(
    void ( QuickFind::*search_function )( Selection, QuickFindMatcher ) )
{
    disableFollow();
    (quickFind_->*search_function )( selection_, quickFindPattern_->getMatcher());
}

void AbstractLogView::setQuickFindResult( bool hasMatch, Portion portion )
{
    if ( portion.isValid() ) {
        LOG( logDEBUG ) << "search " << portion.line();
        displayLine( portion.line() );
        selection_.selectPortion( portion );
        emit updateLineNumber( portion.line() );
    }
    else if ( !hasMatch ) {
        selection_.clear();
    }
}

void AbstractLogView::searchForward()
{
    searchUsingFunction( &QuickFind::searchForward );
}

void AbstractLogView::searchBackward()
{
    searchUsingFunction( &QuickFind::searchBackward );
}

void AbstractLogView::incrementallySearchForward()
{
    searchUsingFunction( &QuickFind::incrementallySearchForward );
}

void AbstractLogView::incrementallySearchBackward()
{
    searchUsingFunction( &QuickFind::incrementallySearchBackward );
}

void AbstractLogView::incrementalSearchAbort()
{
    selection_ = quickFind_->incrementalSearchAbort();
    emit changeQuickFind( "", QuickFindMux::Forward );
}

void AbstractLogView::incrementalSearchStop()
{
    auto oldSelection = quickFind_->incrementalSearchStop();
    if ( selection_.isEmpty() ) {
        selection_ = std::move( oldSelection );
    }
}

void AbstractLogView::followSet( bool checked )
{
    followMode_ = checked;
    followElasticHook_.hook( checked );
    update();
    if ( checked )
        jumpToBottom();
}

void AbstractLogView::refreshOverview()
{
    assert( overviewWidget_ );

    // Create space for the Overview if needed
    if ( ( getOverview() != nullptr ) && getOverview()->isVisible() ) {
        setViewportMargins( 0, 0, OVERVIEW_WIDTH, 0 );
        overviewWidget_->show();
    }
    else {
        setViewportMargins( 0, 0, 0, 0 );
        overviewWidget_->hide();
    }
}

// Reset the QuickFind when the pattern is changed.
void AbstractLogView::handlePatternUpdated()
{
    LOG( logDEBUG ) << "AbstractLogView::handlePatternUpdated()";

    quickFind_->resetLimits();
    update();
}

// OR the current with the current search expression
void AbstractLogView::addToSearch()
{
    if ( selection_.isPortion() ) {
        LOG( logDEBUG ) << "AbstractLogView::addToSearch()";
        emit addToSearch( selection_.getSelectedText( logData ) );
    }
    else {
        LOG( logERROR ) << "AbstractLogView::addToSearch called for a wrong type of selection";
    }
}

// Find next occurrence of the selected text (*)
void AbstractLogView::findNextSelected()
{
    // Use the selected 'word' and search forward
    if ( selection_.isPortion() ) {
        emit changeQuickFind( selection_.getSelectedText( logData ), QuickFindMux::Forward );
        emit searchNext();
    }
}

// Find next previous of the selected text (#)
void AbstractLogView::findPreviousSelected()
{
    if ( selection_.isPortion() ) {
        emit changeQuickFind( selection_.getSelectedText( logData ), QuickFindMux::Backward );
        emit searchNext();
    }
}

// Copy the selection to the clipboard
void AbstractLogView::copy()
{
    static QClipboard* clipboard = QApplication::clipboard();

    clipboard->setText( selection_.getSelectedText( logData ) );
}

void AbstractLogView::saveToFile()
{
    auto filename = QFileDialog::getSaveFileName( this, "Save content" );
    if ( filename.isEmpty() ) {
        return;
    }

    const auto totalLines = logData->getNbLine();
    QSaveFile saveFile{ filename };
    saveFile.open( QIODevice::WriteOnly | QIODevice::Truncate );
    if ( !saveFile.isOpen() ) {
        LOG( logERROR ) << "Failed to open file to save";
        return;
    }

    QProgressDialog progressDialog;
    progressDialog.setLabelText( QString( "Saving content to %1" ).arg( filename ) );

    QFutureWatcher<void> futureWatcher;
    QObject::connect( &futureWatcher, &QFutureWatcher<void>::finished, &progressDialog,
                      &QProgressDialog::reset );
    QObject::connect( &futureWatcher, &QFutureWatcher<void>::progressRangeChanged, &progressDialog,
                      &QProgressDialog::setRange );
    QObject::connect( &futureWatcher, &QFutureWatcher<void>::progressValueChanged, &progressDialog,
                      &QProgressDialog::setValue );

    QObject::connect( &progressDialog, &QProgressDialog::canceled, &futureWatcher,
                      &QFutureWatcher<void>::cancel );

    std::vector<std::pair<LineNumber, LinesCount>> offsets;
    auto lineOffset = 0_lnum;
    const auto chunkSize = 5000_lcount;

    for ( ; lineOffset + chunkSize < LineNumber( totalLines.get() );
          lineOffset += LineNumber( chunkSize.get() ) ) {
        offsets.emplace_back( lineOffset, chunkSize );
    }
    offsets.emplace_back( lineOffset, LinesCount( totalLines.get() % chunkSize.get() ) );

    QTextCodec* codec = logData->getDisplayEncoding();
    if ( !codec ) {
        codec = QTextCodec::codecForName( "utf-8" );
    }

    auto writeLines = [this, &saveFile, &progressDialog, codec]( const auto& offset ) {
        auto lines = logData->getLines( offset.first, offset.second );
        for ( auto& l : lines ) {
            l.append( QChar::LineFeed );
            const auto encodedLine = codec->fromUnicode( l );
            const auto written = saveFile.write( encodedLine );
            if ( written != encodedLine.size() ) {
                LOG( logERROR ) << "Saving file write failed";
                QMetaObject::invokeMethod( &progressDialog, SLOT( cancel() ),
                                           Qt::QueuedConnection );
                return false;
            }
        }
        return true;
    };

    const auto maxThreadCount = QThreadPool::globalInstance()->maxThreadCount();
    QThreadPool::globalInstance()->setMaxThreadCount( 1 );
    futureWatcher.setFuture( QtConcurrent::map( offsets, writeLines ) );

    progressDialog.exec();
    futureWatcher.waitForFinished();

    QThreadPool::globalInstance()->setMaxThreadCount( maxThreadCount );

    if ( futureWatcher.isFinished() ) {
        saveFile.commit();
    }
    else {
        LOG( logERROR ) << "Saving file write failed";
    }
}

void AbstractLogView::updateSearchLimits()
{
    textAreaCache_.invalid_ = true;
    update();

    emit changeSearchLimits( searchStart_, searchEnd_ );
}

void AbstractLogView::setSearchStart()
{
    const auto selectedLine = selection_.selectedLine();
    searchStart_
        = selectedLine.has_value() ? displayLineNumber( *selectedLine ) - 1_lcount : 0_lnum;
    updateSearchLimits();
}

void AbstractLogView::setSearchEnd()
{
    const auto selectedLine = selection_.selectedLine();
    searchEnd_ = selectedLine.has_value() ? displayLineNumber( *selectedLine )
                                          : LineNumber( logData->getNbLine().get() );
    updateSearchLimits();
}

//
// Public functions
//

void AbstractLogView::updateData()
{
    LOG( logDEBUG ) << "AbstractLogView::updateData";

    const auto lastLineNumber = LineNumber( logData->getNbLine().get() );

    // Check the top Line is within range
    if ( firstLine >= lastLineNumber ) {
        firstLine = 0_lnum;
        firstCol = 0;
        verticalScrollBar()->setValue( 0 );
        horizontalScrollBar()->setValue( 0 );
    }

    // Crop selection if it become out of range
    selection_.crop( lastLineNumber - 1_lcount );

    // Adapt the scroll bars to the new content
    updateScrollBars();

    // Calculate the index of the last line shown
    LineNumber last_line = qMin( lastLineNumber, firstLine + getNbVisibleLines() );

    // Reset the QuickFind in case we have new stuff to search into
    quickFind_->resetLimits();

    if ( followMode_ )
        jumpToBottom();

    // Update the overview if we have one
    if ( overview_ != nullptr )
        overview_->updateCurrentPosition( firstLine, last_line );

    textAreaCache_.invalid_ = true;
    update();
}

void AbstractLogView::updateDisplaySize()
{
    // Font is assumed to be mono-space (is restricted by options dialog)
    QFontMetrics fm = fontMetrics();
    charHeight_ = fm.height();
#if ( QT_VERSION >= QT_VERSION_CHECK( 5, 11, 0 ) )
    charWidth_ = fm.horizontalAdvance( QChar( 'a' ) );
#else
    charWidth_ = fm.width( QChar( 'a' ) );
#endif

    // Update the scroll bars
    updateScrollBars();
    verticalScrollBar()->setPageStep( getNbVisibleLines().get() );

    if ( followMode_ )
        jumpToBottom();

    LOG( logDEBUG ) << "viewport.width()=" << viewport()->width();
    LOG( logDEBUG ) << "viewport.height()=" << viewport()->height();
    LOG( logDEBUG ) << "width()=" << width();
    LOG( logDEBUG ) << "height()=" << height();

    if ( overviewWidget_ )
        overviewWidget_->setGeometry( viewport()->width() + 2, 1, OVERVIEW_WIDTH - 1,
                                      viewport()->height() );

    // Our text area cache is now invalid
    textAreaCache_.invalid_ = true;
    textAreaCache_.pixmap_ = QPixmap{ viewport()->width() * viewport()->devicePixelRatio(),
                                      static_cast<int32_t>( getNbVisibleLines().get() )
                                          * charHeight_ * viewport()->devicePixelRatio() };
    textAreaCache_.pixmap_.setDevicePixelRatio( viewport()->devicePixelRatio() );
}

LineNumber AbstractLogView::getTopLine() const
{
    return firstLine;
}

QString AbstractLogView::getSelection() const
{
    return selection_.getSelectedText( logData );
}

bool AbstractLogView::isPartialSelection() const
{
    return selection_.isPortion();
}

void AbstractLogView::selectAll()
{
    selection_.selectRange( 0_lnum, LineNumber( logData->getNbLine().get() ) - 1_lcount );
    textAreaCache_.invalid_ = true;
    update();
}

void AbstractLogView::selectAndDisplayLine( LineNumber line )
{
    disableFollow();
    selection_.selectLine( line );
    displayLine( line );
    emit updateLineNumber( line );
    emit newSelection( line );
}

// The difference between this function and displayLine() is quite
// subtle: this one always jump, even if the line passed is visible.
void AbstractLogView::jumpToLine( LineNumber line )
{
    // Put the selected line in the middle if possible
    const auto newTopLine = line - LinesCount( getNbVisibleLines().get() / 2 );
    // This will also trigger a scrollContents event
    verticalScrollBar()->setValue( newTopLine.get() );
}

void AbstractLogView::setLineNumbersVisible( bool lineNumbersVisible )
{
    lineNumbersVisible_ = lineNumbersVisible;
}

void AbstractLogView::forceRefresh()
{
    // Invalidate our cache
    textAreaCache_.invalid_ = true;
}

void AbstractLogView::setSearchLimits( LineNumber startLine, LineNumber endLine )
{
    searchStart_ = startLine;
    searchEnd_ = endLine;

    textAreaCache_.invalid_ = true;
    update();
}

//
// Private functions
//

// Returns the number of lines visible in the viewport
LinesCount AbstractLogView::getNbVisibleLines() const
{
    return LinesCount( viewport()->height() / charHeight_ + 1 );
}

// Returns the number of columns visible in the viewport
int AbstractLogView::getNbVisibleCols() const
{
    return ( viewport()->width() - leftMarginPx_ ) / charWidth_ + 1;
}

// Converts the mouse x, y coordinates to the line number in the file
OptionalLineNumber AbstractLogView::convertCoordToLine( int yPos ) const
{
    const int line = firstLine.get() + ( yPos - drawingTopOffset_ ) / charHeight_;
    return line >= 0
               ? OptionalLineNumber( LineNumber{ static_cast<LineNumber::UnderlyingType>( line ) } )
               : OptionalLineNumber{};
}

// Converts the mouse x, y coordinates to the char coordinates (in the file)
// This function ensure the pos exists in the file.
QPoint AbstractLogView::convertCoordToFilePos( const QPoint& pos ) const
{
    auto line = convertCoordToLine( pos.y() ).value_or( LineNumber{} );
    if ( line >= logData->getNbLine() )
        line = LineNumber( logData->getNbLine().get() ) - 1_lcount;

    // Determine column in screen space and convert it to file space
    int column = firstCol + ( pos.x() - leftMarginPx_ ) / charWidth_;

    const auto length = static_cast<decltype( column )>( logData->getLineLength( line ).get() );

    if ( column >= length )
        column = length - 1;
    if ( column < 0 )
        column = 0;

    LOG( logDEBUG4 ) << "AbstractLogView::convertCoordToFilePos col=" << column << " line=" << line;
    QPoint point( column, line.get() );

    return point;
}

// Makes the widget adjust itself to display the passed line.
// Doing so, it will throw itself a scrollContents event.
void AbstractLogView::displayLine( LineNumber line )
{
    // If the line is already the screen
    if ( ( line >= firstLine ) && ( line < ( firstLine + getNbVisibleLines() ) ) ) {
        // Invalidate our cache
        textAreaCache_.invalid_ = true;

        // ... don't scroll and just repaint
        update();
    }
    else {
        jumpToLine( line );
    }

    int start_column, end_column;
    if ( selection_.getPortionForLine( line, &start_column, &end_column ) ) {
        horizontalScrollBar()->setValue( end_column - getNbVisibleCols() + 1 );
    }
}

// Move the selection up and down by the passed number of lines
void AbstractLogView::moveSelection( int delta )
{
    LOG( logDEBUG ) << "AbstractLogView::moveSelection delta=" << delta;

    auto selection = selection_.getLines();
    LineNumber new_line;

    if ( !selection.empty() ) {
        if ( delta < 0 )
            new_line = selection.front() - LinesCount( qAbs( delta ) );
        else
            new_line = selection.back() + LinesCount( delta );
    }

    if ( new_line >= logData->getNbLine() ) {
        new_line = LineNumber( logData->getNbLine().get() ) - 1_lcount;
    }

    // Select and display the new line
    selection_.selectLine( new_line );
    displayLine( new_line );
    emit updateLineNumber( new_line );
    emit newSelection( new_line );
}

// Make the start of the lines visible
void AbstractLogView::jumpToStartOfLine()
{
    horizontalScrollBar()->setValue( 0 );
}

// Make the end of the lines in the selection visible
void AbstractLogView::jumpToEndOfLine()
{
    const auto selection = selection_.getLines();

    // Search the longest line in the selection
    const auto max_length = std::accumulate(
        selection.cbegin(), selection.cend(), 0_length, [this]( auto currentMax, auto line ) {
            return qMax( currentMax, logData->getLineLength( LineNumber( line ) ) );
        } );
    horizontalScrollBar()->setValue( max_length.get() - getNbVisibleCols() );
}

// Make the end of the lines on the screen visible
void AbstractLogView::jumpToRightOfScreen()
{
    // Search the longest line on screen
    auto max_length = 0_length;

    const auto nbVisibleLines = getNbVisibleLines();
    for ( auto i = firstLine; i <= ( firstLine + nbVisibleLines ); ++i ) {
        const auto length = logData->getLineLength( i );
        max_length = qMax( length, max_length );
    }

    horizontalScrollBar()->setValue( max_length.get() - getNbVisibleCols() );
}

// Jump to the first line
void AbstractLogView::jumpToTop()
{
    // This will also trigger a scrollContents event
    verticalScrollBar()->setValue( 0 );
    update(); // in case the screen hasn't moved
}

// Jump to the last line
void AbstractLogView::jumpToBottom()
{
    const auto new_top_line
        = qMax( logData->getNbLine().get() - getNbVisibleLines().get() + 1, 0U );

    // This will also trigger a scrollContents event
    verticalScrollBar()->setValue( new_top_line );
    update(); // in case the screen hasn't moved
}

// Returns whether the character passed is a 'word' character
inline bool AbstractLogView::isCharWord( char c )
{
    if ( ( ( c >= 'A' ) && ( c <= 'Z' ) ) || ( ( c >= 'a' ) && ( c <= 'z' ) )
         || ( ( c >= '0' ) && ( c <= '9' ) ) || ( ( c == '_' ) ) )
        return true;
    else
        return false;
}

// Select the word under the given position
void AbstractLogView::selectWordAtPosition( const QPoint& pos )
{
    const int x = pos.x();
    const auto lineNumber = LineNumber( pos.y() );
    const QString line = logData->getExpandedLineString( lineNumber );

    if ( isCharWord( line[ x ].toLatin1() ) ) {
        // Search backward for the first character in the word
        int currentPos = x;
        for ( ; currentPos > 0; currentPos-- )
            if ( !isCharWord( line[ currentPos ].toLatin1() ) )
                break;
        // Exclude the first char of the line if needed
        if ( !isCharWord( line[ currentPos ].toLatin1() ) )
            currentPos++;
        int start = currentPos;

        // Now search for the end
        currentPos = x;
        for ( ; currentPos < line.length() - 1; currentPos++ )
            if ( !isCharWord( line[ currentPos ].toLatin1() ) )
                break;
        // Exclude the last char of the line if needed
        if ( !isCharWord( line[ currentPos ].toLatin1() ) )
            currentPos--;
        int end = currentPos;

        selection_.selectPortion( lineNumber, start, end );
        updateGlobalSelection();
        update();
    }
}

// Update the system global (middle click) selection (X11 only)
void AbstractLogView::updateGlobalSelection()
{
    static QClipboard* const clipboard = QApplication::clipboard();

    // Updating it only for "non-trivial" (range or portion) selections
    if ( !selection_.isSingleLine() )
        clipboard->setText( selection_.getSelectedText( logData ), QClipboard::Selection );
}

// Create the pop-up menu
void AbstractLogView::createMenu()
{
    copyAction_ = new QAction( tr( "&Copy" ), this );
    // No text as this action title depends on the type of selection
    connect( copyAction_, &QAction::triggered, [this]( auto ) { this->copy(); } );

    saveToFileAction_ = new QAction( tr( "Save to file" ), this );
    connect( saveToFileAction_, &QAction::triggered, [this]( auto ) { this->saveToFile(); } );

    // For '#' and '*', shortcuts doesn't seem to work but
    // at least it displays them in the menu, we manually handle those keys
    // as keys event anyway (in keyPressEvent).
    findNextAction_ = new QAction( tr( "Find &next" ), this );
    findNextAction_->setShortcut( Qt::Key_Asterisk );
    findNextAction_->setStatusTip( tr( "Find the next occurrence" ) );
    connect( findNextAction_, &QAction::triggered, [this]( auto ) { this->findNextSelected(); } );

    findPreviousAction_ = new QAction( tr( "Find &previous" ), this );
    findPreviousAction_->setShortcut( tr( "/" ) );
    findPreviousAction_->setStatusTip( tr( "Find the previous occurrence" ) );
    connect( findPreviousAction_, &QAction::triggered,
             [this]( auto ) { this->findPreviousSelected(); } );

    addToSearchAction_ = new QAction( tr( "&Add to search" ), this );
    addToSearchAction_->setStatusTip( tr( "Add the selection to the current search" ) );
    connect( addToSearchAction_, &QAction::triggered, [this]( auto ) { this->addToSearch(); } );

    setSearchStartAction_ = new QAction( tr( "Set search start" ), this );
    connect( setSearchStartAction_, &QAction::triggered,
             [this]( auto ) { this->setSearchStart(); } );

    setSearchEndAction_ = new QAction( tr( "Set search end" ), this );
    connect( setSearchEndAction_, &QAction::triggered, [this]( auto ) { this->setSearchEnd(); } );

    clearSearchLimitAction_ = new QAction( tr( "Clear search limits" ), this );
    connect( clearSearchLimitAction_, &QAction::triggered,
             [this]( auto ) { this->clearSearchLimits(); } );

    saveDefaultSplitterSizesAction_ = new QAction( tr( "Save splitter position" ), this );
    connect( saveDefaultSplitterSizesAction_, &QAction::triggered,
             [this]( auto ) { this->saveDefaultSplitterSizes(); } );

    popupMenu_ = new QMenu( this );
    popupMenu_->addAction( copyAction_ );
    popupMenu_->addAction( saveToFileAction_ );
    popupMenu_->addSeparator();
    popupMenu_->addAction( findNextAction_ );
    popupMenu_->addAction( findPreviousAction_ );
    popupMenu_->addAction( addToSearchAction_ );
    popupMenu_->addSeparator();
    popupMenu_->addAction( setSearchStartAction_ );
    popupMenu_->addAction( setSearchEndAction_ );
    popupMenu_->addAction( clearSearchLimitAction_ );
    popupMenu_->addSeparator();
    popupMenu_->addAction( saveDefaultSplitterSizesAction_ );
}

void AbstractLogView::considerMouseHovering( int x_pos, int y_pos )
{
    const auto line = convertCoordToLine( y_pos );
    if ( ( x_pos < leftMarginPx_ ) && ( line.has_value() ) && ( *line < logData->getNbLine() ) ) {
        // Mouse moved in the margin, send event up
        // (possibly to highlight the overview)
        if ( line != lastHoveredLine_ ) {
            LOG( logDEBUG ) << "Mouse moved in margin line: " << line;
            emit mouseHoveredOverLine( *line );
            lastHoveredLine_ = line;
        }
    }
    else {
        if ( lastHoveredLine_.has_value() ) {
            emit mouseLeftHoveringZone();
            lastHoveredLine_ = {};
        }
    }
}

void AbstractLogView::updateScrollBars()
{
    verticalScrollBar()->setRange(
        0, qMax( 0U, logData->getNbLine().get() - getNbVisibleLines().get() + 1 ) );

    const int hScrollMaxValue = qMax( 0, logData->getMaxLength().get() - getNbVisibleCols() + 1 );

    horizontalScrollBar()->setRange( 0, hScrollMaxValue );
}

void AbstractLogView::drawTextArea( QPaintDevice* paint_device, int32_t delta_y )
{
    Q_UNUSED( delta_y );
    // LOG( logDEBUG ) << "devicePixelRatio: " << viewport()->devicePixelRatio();
    // LOG( logDEBUG ) << "viewport size: " << viewport()->size().width();
    // LOG( logDEBUG ) << "pixmap size: " << textPixmap.width();
    // Repaint the viewport
    QPainter painter( paint_device );
    // LOG( logDEBUG ) << "font: " << viewport()->font().family().toStdString();
    // LOG( logDEBUG ) << "font painter: " << painter.font().family().toStdString();

    painter.setFont( this->font() );

    const int fontHeight = charHeight_;
    const int fontAscent = painter.fontMetrics().ascent();
    const int nbCols = getNbVisibleCols();
    const int paintDeviceHeight = paint_device->height() / viewport()->devicePixelRatio();
    const int paintDeviceWidth = paint_device->width() / viewport()->devicePixelRatio();
    const QPalette& palette = viewport()->palette();
    const auto& highlighterSet = HighlighterSet::get();
    QColor foreColor, backColor;

    static const QBrush normalBulletBrush = QBrush( Qt::white );
    static const QBrush matchBulletBrush = QBrush( Qt::red );
    static const QBrush markBrush = QBrush( "dodgerblue" );
    static const QBrush markedMatchBrush = QBrush( "maroon" );

    static const int SEPARATOR_WIDTH = 1;
    static const qreal BULLET_AREA_WIDTH = 11;
    static const int CONTENT_MARGIN_WIDTH = 1;
    static const int LINE_NUMBER_PADDING = 3;

    // First check the lines to be drawn are within range (might not be the case if
    // the file has just changed)
    const auto lines_in_file = logData->getNbLine();

    if ( firstLine > lines_in_file )
        firstLine = LineNumber( lines_in_file.get() ? lines_in_file.get() - 1 : 0 );

    const auto nbLines = qMin( getNbVisibleLines(), lines_in_file - LinesCount( firstLine.get() ) );

    const int bottomOfTextPx = nbLines.get() * fontHeight;

    LOG( logDEBUG1 ) << "drawing lines from " << firstLine << " (" << nbLines << " lines)";
    LOG( logDEBUG1 ) << "bottomOfTextPx: " << bottomOfTextPx;
    LOG( logDEBUG1 ) << "Height: " << paintDeviceHeight;

    // Lines to write
    const auto lines = logData->getExpandedLines( firstLine, nbLines );

    // First draw the bullet left margin
    painter.setPen( palette.color( QPalette::Text ) );
    painter.fillRect( 0, 0, BULLET_AREA_WIDTH, paintDeviceHeight, Qt::darkGray );

    // Column at which the content should start (pixels)
    qreal contentStartPosX = BULLET_AREA_WIDTH + SEPARATOR_WIDTH;

    // This is also the bullet zone width, used for marking clicks
    bulletZoneWidthPx_ = contentStartPosX;

    // Update the length of line numbers
    const int nbDigitsInLineNumber = countDigits( maxDisplayLineNumber().get() );

    // Draw the line numbers area
    int lineNumberAreaStartX = 0;
    if ( lineNumbersVisible_ ) {
        int lineNumberWidth = charWidth_ * nbDigitsInLineNumber;
        int lineNumberAreaWidth = 2 * LINE_NUMBER_PADDING + lineNumberWidth;
        lineNumberAreaStartX = contentStartPosX;

        painter.setPen( palette.color( QPalette::Text ) );
        /* Not sure if it looks good...
        painter.drawLine( contentStartPosX + lineNumberAreaWidth,
                          0,
                          contentStartPosX + lineNumberAreaWidth,
                          viewport()->height() );
        */
        painter.fillRect( contentStartPosX - SEPARATOR_WIDTH, 0,
                          lineNumberAreaWidth + SEPARATOR_WIDTH, paintDeviceHeight, Qt::lightGray );

        // Update for drawing the actual text
        contentStartPosX += lineNumberAreaWidth;
    }
    else {
        painter.fillRect( contentStartPosX - SEPARATOR_WIDTH, 0, SEPARATOR_WIDTH + 1,
                          paintDeviceHeight, Qt::lightGray );
        // contentStartPosX += SEPARATOR_WIDTH;
    }

    painter.drawLine( BULLET_AREA_WIDTH, 0, BULLET_AREA_WIDTH, paintDeviceHeight - 1 );

    // This is the total width of the 'margin' (including line number if any)
    // used for mouse calculation etc...
    leftMarginPx_ = contentStartPosX + SEPARATOR_WIDTH;

    const auto searchStartIndex = lineIndex( searchStart_ );
    const auto searchEndIndex = [this] {
            auto index = lineIndex( searchEnd_ );
            if (searchEnd_ + 1_lcount != displayLineNumber(index)) {
                // in filtered view lineIndex for "past the end" returns last line
                // it should not be marked as excluded
                index = index + 1_lcount;
            }

            return index;
    }();

    // Then draw each line
    for ( auto i = 0_lcount; i < nbLines; ++i ) {
        const auto line_index = firstLine + i;

        // Position in pixel of the base line of the line to print
        const int yPos = i.get() * fontHeight;
        const int xPos = contentStartPosX + CONTENT_MARGIN_WIDTH;

        // string to print, cut to fit the length and position of the view
        const QString line = lines[ i.get() ];
        const QString cutLine = line.mid( firstCol, nbCols );

        if ( selection_.isLineSelected( line_index ) ) {
            // Reverse the selected line
            foreColor = palette.color( QPalette::HighlightedText );
            backColor = palette.color( QPalette::Highlight );
            painter.setPen( palette.color( QPalette::Text ) );
        }
        else if ( highlighterSet.matchLine( logData->getLineString( line_index ), &foreColor,
                                            &backColor ) ) {
            // Apply a filter to the line
        }
        else {
            // Use the default colors
            if ( line_index < searchStartIndex || line_index >= searchEndIndex ) {
                foreColor = palette.brush( QPalette::Disabled, QPalette::Text ).color();
            }
            else {
                foreColor = palette.color( QPalette::Text );
            }

            backColor = palette.color( QPalette::Base );
        }

        // Is there something selected in the line?
        int sel_start, sel_end;
        bool isSelection = selection_.getPortionForLine( line_index, &sel_start, &sel_end );
        // Has the line got elements to be highlighted
        std::vector<QuickFindMatch> qfMatchList;
        bool isMatch = quickFindPattern_->matchLine( line, qfMatchList );

        if ( isSelection || isMatch ) {
            // We use the LineDrawer and its chunks because the
            // line has to be somehow highlighted
            LineDrawer lineDrawer( backColor );

            // First we create a list of chunks with the highlights
            std::vector<LineChunk> chunkList;
            int column = 0; // Current column in line space
            for ( const auto& match : qfMatchList ) {
                int start = match.startColumn() - firstCol;
                int end = start + match.length();

                // Ignore matches that are *completely* outside view area
                if ( ( start < 0 && end < 0 ) || start >= nbCols )
                    continue;

                if ( start > column )
                    chunkList.emplace_back( column, start - 1, LineChunk::Normal );

                column = qMin( start + match.length() - 1, nbCols );
                chunkList.emplace_back( qMax( start, 0 ), column, LineChunk::Highlighted );

                column++;
            }
            if ( column <= cutLine.length() - 1 )
                chunkList.emplace_back( column, cutLine.length() - 1, LineChunk::Normal );

            // Then we add the selection if needed
            std::vector<LineChunk> newChunkList;
            if ( isSelection ) {
                sel_start -= firstCol; // coord in line space
                sel_end -= firstCol;

                for ( const auto& chunk : chunkList ) {
                    auto selection = chunk.select( sel_start, sel_end );
                    newChunkList.insert( newChunkList.end(),
                                         std::make_move_iterator( selection.begin() ),
                                         std::make_move_iterator( selection.end() ) );
                }
            }
            else {
                newChunkList = chunkList;
            }

            for ( const auto& chunk : newChunkList ) {
                // Select the colours
                QColor fore;
                QColor back;
                switch ( chunk.type() ) {
                case LineChunk::Normal:
                    fore = foreColor;
                    back = backColor;
                    break;
                case LineChunk::Highlighted:
                    fore = QColor( "black" );
                    back = QColor( "yellow" );
                    // fore = highlightForeColor;
                    // back = highlightBackColor;
                    break;
                case LineChunk::Selected:
                    fore = palette.color( QPalette::HighlightedText );
                    back = palette.color( QPalette::Highlight );
                    break;
                }
                lineDrawer.addChunk( chunk, fore, back );
            }

            lineDrawer.draw( painter, xPos, yPos, viewport()->width(), cutLine,
                             CONTENT_MARGIN_WIDTH );
        }
        else {
            // Nothing to be highlighted, we print the whole line!
            painter.fillRect( xPos - CONTENT_MARGIN_WIDTH, yPos, viewport()->width(), fontHeight,
                              backColor );
            // (the rectangle is extended on the left to cover the small
            // margin, it looks better (LineDrawer does the same) )
            painter.setPen( foreColor );
            painter.drawText( xPos, yPos + fontAscent, cutLine );
        }

        // Then draw the bullet
        painter.setPen( palette.color( QPalette::Text ) );
        const qreal circleSize = 3;
        const qreal arrowHeight = 4;
        const qreal middleXLine = BULLET_AREA_WIDTH / 2;
        const qreal middleYLine = yPos + ( fontHeight / 2 );

        using LineTypeFlags = AbstractLogData::LineTypeFlags;
        const AbstractLogData::LineType line_type = lineType( line_index );
        if ( line_type.testFlag( LineTypeFlags::Mark ) ) {
            // A pretty arrow if the line is marked
            const QPointF points[ 7 ] = {
                QPointF( 1, middleYLine - 2 ),
                QPointF( middleXLine, middleYLine - 2 ),
                QPointF( middleXLine, middleYLine - arrowHeight ),
                QPointF( BULLET_AREA_WIDTH - 1, middleYLine ),
                QPointF( middleXLine, middleYLine + arrowHeight ),
                QPointF( middleXLine, middleYLine + 2 ),
                QPointF( 1, middleYLine + 2 ),
            };

            painter.setBrush( line_type.testFlag( LineTypeFlags::Match ) ? markedMatchBrush : markBrush );
            painter.drawPolygon( points, 7 );
        }
        else {
            // For pretty circles
            painter.setRenderHint( QPainter::Antialiasing );

            QBrush brush = normalBulletBrush;;
            if ( line_type.testFlag( LineTypeFlags::Match ) )
                brush = matchBulletBrush;
            painter.setBrush( brush );
            painter.drawEllipse( middleXLine - circleSize, middleYLine - circleSize, circleSize * 2,
                                 circleSize * 2 );
        }

        // Draw the line number
        if ( lineNumbersVisible_ ) {
            static const QString lineNumberFormat( "%1" );
            const QString& lineNumberStr = lineNumberFormat.arg(
                displayLineNumber( line_index ).get(), nbDigitsInLineNumber );
            painter.setPen( palette.color( QPalette::Text ) );
            painter.drawText( lineNumberAreaStartX + LINE_NUMBER_PADDING, yPos + fontAscent,
                              lineNumberStr );
        }
    } // For each line

    if ( bottomOfTextPx < paintDeviceHeight ) {
        // The lines don't cover the whole device
        painter.fillRect( contentStartPosX, bottomOfTextPx, paintDeviceWidth - contentStartPosX,
                          paintDeviceHeight, palette.color( QPalette::Window ) );
    }
}

// Draw the "pull to follow" bar and return a pixmap.
// The width is passed in "logic" pixels.
QPixmap AbstractLogView::drawPullToFollowBar( int width, float pixel_ratio )
{
    static constexpr int barWidth = 40;
    QPixmap pixmap( static_cast<float>( width ) * pixel_ratio, barWidth * 6.0 );
    pixmap.setDevicePixelRatio( pixel_ratio );
    pixmap.fill( this->palette().color( this->backgroundRole() ) );
    const int nbBars = width / ( barWidth * 2 ) + 1;

    QPainter painter( &pixmap );
    painter.setPen( QPen( QColor( 0, 0, 0, 0 ) ) );
    painter.setBrush( QBrush( QColor( "lightyellow" ) ) );

    for ( int i = 0; i < nbBars; ++i ) {
        QPoint points[ 4 ] = { { ( i * 2 + 1 ) * barWidth, 0 },
                               { 0, ( i * 2 + 1 ) * barWidth },
                               { 0, ( i + 1 ) * 2 * barWidth },
                               { ( i + 1 ) * 2 * barWidth, 0 } };
        painter.drawConvexPolygon( points, 4 );
    }

    return pixmap;
}

void AbstractLogView::disableFollow()
{
    emit followModeChanged( false );
    followElasticHook_.hook( false );
}

namespace {

// Convert the length of the pull to follow bar to pixels
int mapPullToFollowLength( int length )
{
    return length / 14;
}

} // namespace
