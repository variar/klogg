/*
 * Copyright (C) 2009, 2010, 2011, 2012, 2013, 2017 Nicolas Bonnefon
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
 * Copyright (C) 2016 -- 2021 Anton Filimonov and other contributors
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

#ifndef ABSTRACTLOGVIEW_H
#define ABSTRACTLOGVIEW_H

#include <array>
#include <cstddef>
#include <functional>
#include <string_view>
#include <vector>

#include <QAbstractScrollArea>
#include <QBasicTimer>
#include <QColor>
#include <QEvent>
#include <QFontMetrics>

#ifdef GLOGG_PERF_MEASURE_FPS
#include "perfcounter.h"
#endif

#include "abstractlogdata.h"
#include "linetypes.h"
#include "overviewwidget.h"
#include "quickfind.h"
#include "quickfindmux.h"
#include "regularexpressionpattern.h"
#include "selection.h"
#include "viewtools.h"

class QMenu;
class QAction;
class QShortcut;

class LineChunk {
  public:
    LineChunk( int firstCol, int endCol, QColor foreColor, QColor backColor )
        : start_{ firstCol }
        , end_{ endCol }
        , foreColor_{ foreColor }
        , backColor_{ backColor }
    {
    }

    int start() const
    {
        return start_;
    }
    int end() const
    {
        return end_;
    }

    int length() const
    {
        return end_ - start_ + 1;
    }

    QColor foreColor() const
    {
        return foreColor_;
    }

    QColor backColor() const
    {
        return backColor_;
    }

  private:
    int start_ = {};
    int end_ = {};

    QColor foreColor_;
    QColor backColor_;
};

// Utility class for syntax colouring.
// It stores the chunks of line to draw
// each chunk having a different colour
class LineDrawer {
  public:
    explicit LineDrawer( const QColor& backColor )
        : backColor_( backColor )
    {
    }

    // Add a chunk of line using the given colours.
    // Both first_col and last_col are included
    // An empty chunk will be ignored.
    // the first column will be set to 0 if negative
    // The column are relative to the screen
    void addChunk( int firstCol, int lastCol, const QColor& fore, const QColor& back );
    void addChunk( const LineChunk& chunk );

    // Draw the current line of text using the given painter,
    // in the passed block (in pixels)
    // The line must be cut to fit on the screen.
    // leftExtraBackgroundPx is the an extra margin to start drawing
    // the coloured // background, going all the way to the element
    // left of the line looks better.
    void draw( QPainter* painter, int xPos, int yPos, int lineWidth, const QString& line,
               int leftExtraBackgroundPx );

  private:
    std::vector<LineChunk> chunks_;
    QColor backColor_;
};

// Utility class representing a buffer for number entered on the keyboard
// The buffer keep at most 7 digits, and reset itself after a timeout.
class DigitsBuffer : public QObject {
    Q_OBJECT

  public:
    // Reset the buffer.
    void reset();
    // Add a single digit to the buffer (discarded if it's not a digit),
    // the timeout timer is reset.
    void add( char character );
    // Get the content of the buffer (0 if empty) and reset it.
    LineNumber::UnderlyingType content();

    bool isEmpty() const;

  protected:
    void timerEvent( QTimerEvent* event ) override;

  private:
    // Duration of the timeout in milliseconds.
    static constexpr int DigitsTimeout = 2000;

    QString digits_;

    QBasicTimer timer_;
};

class Overview;

// Base class representing the log view widget.
// It can be either the top (full) or bottom (filtered) view.
class AbstractLogView : public QAbstractScrollArea, public SearchableWidgetInterface {
    Q_OBJECT

  public:
    // Constructor of the widget, the data set is passed.
    // The caller retains ownership of the data set.
    // The pointer to the QFP is used for colouring and QuickFind searches
    AbstractLogView( const AbstractLogData* newLogData, const QuickFindPattern* const quickFind,
                     QWidget* parent = nullptr );

    ~AbstractLogView() override;

    // rule of 5
    AbstractLogView( const AbstractLogView& ) = delete;
    AbstractLogView( AbstractLogView&& ) = delete;
    AbstractLogView& operator=( const AbstractLogView& ) = delete;
    AbstractLogView& operator=( AbstractLogView&& ) = delete;

    void updateFont( const QFont& font );

    // Refresh the widget when the data set has changed.
    void updateData();
    // Instructs the widget to update it's content geometry,
    // used when the font is changed.
    void updateDisplaySize();
    // Return the line number of the top line of the view
    LineNumber getTopLine() const;
    // Return the text of the current selection.
    QString getSelection() const;
    // True for partial selection
    bool isPartialSelection() const;
    // Instructs the widget to select the whole text.
    void selectAll();

    bool isFollowEnabled() const
    {
        return followMode_;
    }

    void allowFollowMode( bool allow );

    void setSearchPattern( const RegularExpressionPattern& pattern );

    using QuickHighlighters = QStringList;
    void setQuickHighlighters( const std::vector<QuickHighlighters>& wordHighlighters );

    void registerShortcuts();

  protected:
    void mousePressEvent( QMouseEvent* mouseEvent ) override;
    void mouseMoveEvent( QMouseEvent* mouseEvent ) override;
    void mouseReleaseEvent( QMouseEvent* ) override;
    void mouseDoubleClickEvent( QMouseEvent* mouseEvent ) override;
    void timerEvent( QTimerEvent* timerEvent ) override;
    void changeEvent( QEvent* changeEvent ) override;
    void paintEvent( QPaintEvent* paintEvent ) override;
    void resizeEvent( QResizeEvent* resizeEvent ) override;
    void scrollContentsBy( int dx, int dy ) override;
    void keyPressEvent( QKeyEvent* keyEvent ) override;
    void wheelEvent( QWheelEvent* wheelEvent ) override;
    bool event( QEvent* e ) override;

    // Must be implemented to return what LineType the line number is
    // (used for coloured bullets)
    virtual AbstractLogData::LineType lineType( LineNumber lineNumber ) const = 0;

    // Line number to display for line at the given index
    virtual LineNumber displayLineNumber( LineNumber lineNumber ) const;
    virtual LineNumber lineIndex( LineNumber lineNumber ) const;
    virtual LineNumber maxDisplayLineNumber() const;

    // Get the overview associated with this view, or NULL if there is none
    Overview* getOverview() const
    {
        return overview_;
    }
    // Set the Overview and OverviewWidget
    void setOverview( Overview* overview, OverviewWidget* overviewWidget );

    // Returns the current "position" of the view as a line number,
    // it is either the selected line or the middle of the view.
    LineNumber getViewPosition() const;

    virtual void doRegisterShortcuts();
    void registerShortcut( const std::string& action, std::function<void()> func );

  Q_SIGNALS:
    // Sent when a new line has been selected by the user.
    void newSelection( LineNumber line );
    // Sent up to the MainWindow to enable/disable the follow mode
    void followModeChanged( bool enabled );
    // Sent when the view wants the QuickFind widget pattern to change.
    void changeQuickFind( const QString& newPattern, QuickFindMux::QFDirection newDirection );
    // Sent up when the current line number is updated
    void updateLineNumber( LineNumber line );
    // Sent up when quickFind wants to show a message to the user.
    void notifyQuickFind( const QFNotification& message );
    // Sent up when quickFind wants to clear the notification.
    void clearQuickFindNotification();
    // Sent when the view ask for a line to be marked
    // (click in the left margin).
    void markLines( const std::vector<LineNumber>& lines );
    // Sent up when the user wants to add the selection to the search
    void addToSearch( const QString& selection );
    // Sent up when the user wants to replace the search with the selection
    void replaceSearch( const QString& selection );
    void excludeFromSearch( const QString& selection );
    // Sent up when the mouse is hovered over a line's margin
    void mouseHoveredOverLine( LineNumber line );
    // Sent up when the mouse leaves a line's margin
    void mouseLeftHoveringZone();
    // Sent up for view initiated quickfind searches
    void searchNext();
    void searchPrevious();
    // Sent up when the user has moved within the view
    void activity();
    // Sent up when the user want to exit this view
    // (switch to the next one)
    void exitView();

    void changeSearchLimits( LineNumber startLine, LineNumber endLine );
    void clearSearchLimits();

    void saveDefaultSplitterSizes();
    void sendSelectionToScratchpad();
    void replaceScratchpadWithSelection();
    void changeFontSize( bool increase );

    void addColorLabel( size_t label );
    void addNextColorLabel();
    void clearColorLabels();

  public Q_SLOTS:
    // Makes the widget select and display the passed line.
    // Scrolling as necessary
    void trySelectLine( LineNumber newLine );
    void selectAndDisplayLine( LineNumber line );

    // Use the current QFP to go and select the next match.
    void searchForward() override;
    // Use the current QFP to go and select the previous match.
    void searchBackward() override;

    // Use the current QFP to go and select the next match (incremental)
    void incrementallySearchForward() override;
    // Use the current QFP to go and select the previous match (incremental)
    void incrementallySearchBackward() override;
    // Stop the current incremental search (typically when user press return)
    void incrementalSearchStop() override;
    // Abort the current incremental search (typically when user press esc)
    void incrementalSearchAbort() override;

    // Signals the follow mode has been enabled.
    void followSet( bool checked );

    // Signal the on/off status of the overview has been changed.
    void refreshOverview();

    // Make the view jump to the specified line, regardless of it
    // being on the screen or not. (does NOT Q_EMIT followDisabled() )
    void jumpToLine( LineNumber line );

    // Configure the setting of whether to show line number margin
    void setLineNumbersVisible( bool lineNumbersVisible );

    // Force the next refresh to fully redraw the view by invalidating the cache.
    // To be used if the data might have changed.
    void forceRefresh();

    void setSearchLimits( LineNumber startLine, LineNumber endLine );

  private Q_SLOTS:
    void handlePatternUpdated();
    void addToSearch();
    void replaceSearch();
    void excludeFromSearch();
    void findNextSelected();
    void findPreviousSelected();
    void copy();
    void copyWithLineNumbers();
    void markSelected();
    void saveToFile();
    void saveSelectedToFile();
    void setSearchStart();
    void setSearchEnd();
    void setSelectionStart();
    void setSelectionEnd();
    void setQuickFindResult( bool hasMatch, const Portion& selection );
    void setHighlighterSet( QAction* action );
    void setColorLabel( QAction* action );

  private:
    // Graphic parameters
    static constexpr int OverviewWidth = 27;
    static constexpr int HookThreshold = 300;
    static constexpr int PullToFollowHookedHeight = 10;

    // Width of the bullet zone, including decoration
    int bulletZoneWidthPx_;

    // Total size of all margins and decorations in pixels
    int leftMarginPx_ = 0;

    // Digits buffer (for numeric keyboard entry)
    DigitsBuffer digitsBuffer_;

    // Follow mode
    bool followMode_ = false;

    // ElasticHook for follow mode
    ElasticHook followElasticHook_;

    // Whether to show line numbers or not
    bool lineNumbersVisible_ = false;

    // Pointer to the CrawlerWidget's data set
    const AbstractLogData* logData_;

    // Pointer to the Overview object
    Overview* overview_ = nullptr;

    // Pointer to the OverviewWidget, this class doesn't own it,
    // but is responsible for displaying it (for purely aesthetic
    // reasons).
    OverviewWidget* overviewWidget_ = nullptr;

    struct FilePos {
        LineNumber line;
        int column;
    };

    bool selectionStarted_ = false;
    // Start of the selection (characters)
    FilePos selectionStartPos_;
    // Current end of the selection (characters)
    FilePos selectionCurrentEndPos_;
    QBasicTimer autoScrollTimer_;

    // Hovering state
    // Last line that has been hoovered on, -1 if none
    OptionalLineNumber lastHoveredLine_;

    // Marks (left margin click)
    bool markingClickInitiated_ = false;
    OptionalLineNumber markingClickLine_;

    Selection selection_;
    RegularExpressionPattern searchPattern_;

    std::vector<QuickHighlighters> quickHighlighters_ = std::vector<QuickHighlighters>{ 9 };

    // Position of the view, those are crucial to control drawing
    // firstLine gives the position of the view,
    // lastLineAligned == true make the bottom of the last line aligned
    // rather than the top of the top one.
    LineNumber firstLine_;
    bool lastLineAligned_ = false;
    int firstCol_ = 0;

    LineNumber searchStart_;
    LineNumber searchEnd_;

    OptionalLineNumber selectionStart_;

    // Text handling
    int charWidth_ = 1;
    int charHeight_ = 10;

    // Popup menu
    QMenu* popupMenu_;
    QAction* copyAction_;
    QAction* copyWithLineNumbersAction_;
    QAction* markAction_;
    QAction* sendToScratchpadAction_;
    QAction* replaceInScratchpadAction_;
    QAction* saveToFileAction_;
    QAction* saveSelectedToFileAction_;
    QAction* findNextAction_;
    QAction* findPreviousAction_;
    QAction* addToSearchAction_;
    QAction* replaceSearchAction_;
    QAction* excludeFromSearchAction_;
    QAction* setSearchStartAction_;
    QAction* setSearchEndAction_;
    QAction* clearSearchLimitAction_;
    QAction* setSelectionStartAction_;
    QAction* setSelectionEndAction_;
    QAction* saveDefaultSplitterSizesAction_;
    QMenu* highlightersMenu_;
    QMenu* colorLabelsMenu_;

    std::map<QString, QShortcut*> shortcuts_;

    // Pointer to the CrawlerWidget's QFP object
    const QuickFindPattern* const quickFindPattern_;
    // Our own QuickFind object
    QuickFind* quickFind_;

#ifdef GLOGG_PERF_MEASURE_FPS
    // Performance measurement
    PerfCounter perfCounter_;
#endif

    // Vertical offset (in pixels) at which the first line of text is written
    int drawingTopOffset_ = 0;

    // Cache pixmap and associated info
    struct TextAreaCache {
        QPixmap pixmap_;
        bool invalid_;
        LineNumber first_line_;
        LineNumber last_line_;
        int first_column_;
    };
    struct PullToFollowCache {
        QPixmap pixmap_;
        int nb_columns_;
    };
    TextAreaCache textAreaCache_ = { {}, true, 0_lnum, 0_lnum, 0 };
    PullToFollowCache pullToFollowCache_ = { {}, 0 };
    QFontMetrics pixmapFontMetrics_;

    LinesCount getNbVisibleLines() const;
    int getNbVisibleCols() const;

    FilePos convertCoordToFilePos( const QPoint& pos ) const;
    OptionalLineNumber convertCoordToLine( int yPos ) const;
    int convertCoordToColumn( int xPos ) const;

    void displayLine( LineNumber line );
    void moveSelection( LinesCount delta, bool isDeltaNegative );
    void moveSelectionUp();
    void moveSelectionDown();
    void jumpToStartOfLine();
    void jumpToEndOfLine();
    void jumpToRightOfScreen();
    void jumpToTop();
    void jumpToBottom();
    void selectWordAtPosition( const FilePos& pos );

    void updateSearchLimits();

    void createMenu();

    void considerMouseHovering( int xPos, int yPos );

    LineLength maxLineLength(const std::vector<LineNumber>& lines) const;

    // Save specified lines in range [begin, end) to a file
    void saveLinesToFile( LineNumber begin, LineNumber end );

    // Search functions (for n/N)
    using QuickFindSearchFn = void ( QuickFind::* )( Selection, QuickFindMatcher );
    void searchUsingFunction( QuickFindSearchFn searchFunction );

    void updateScrollBars();

    LineNumber verticalScrollToLineNumber( int scrollPosition ) const;
    int lineNumberToVerticalScroll( LineNumber line ) const;
    double verticalScrollMultiplicator() const;

    void drawTextArea( QPaintDevice* paintDevice );
    QPixmap drawPullToFollowBar( int width, qreal pixelRatio );

    void disableFollow();

    // Utils functions
    void updateGlobalSelection();

    void selectAndDisplayRange( FilePos pos );
};

#endif
