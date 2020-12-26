/*
 * Copyright (C) 2009, 2010, 2011, 2013, 2014, 2015 Nicolas Bonnefon
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

#ifndef CRAWLERWIDGET_H
#define CRAWLERWIDGET_H

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QSplitter>
#include <QToolButton>
#include <QVBoxLayout>

#include <absl/types/optional.h>

#include "data/loadingstatus.h"
#include "data/logdata.h"
#include "data/logfiltereddata.h"
#include "filteredview.h"
#include "logmainview.h"
#include "overview.h"
#include "signalmux.h"
#include "viewinterface.h"
#include "iconloader.h"

class InfoLine;
class QuickFindPattern;
class SavedSearches;
class QStandardItemModel;
class QCompleter;
class OverviewWidget;

// Implements the central widget of the application.
// It includes both windows, the search line, the info
// lines and various buttons.
class CrawlerWidget : public QSplitter,
                      public QuickFindMuxSelectorInterface,
                      public ViewInterface,
                      public MuxableDocumentInterface {
    Q_OBJECT

  public:
    CrawlerWidget( QWidget* parent = nullptr );

    // Get the line number of the first line displayed.
    LineNumber getTopLine() const;
    // Get the selected text as a string (from the main window)
    QString getSelectedText() const;
    // True for partial selection
    bool isPartialSelection() const;

    // Display the QFB at the bottom, remembering where the focus was
    void displayQuickFindBar( QuickFindMux::QFDirection direction );

    // Instructs the widget to select all the text in the window the user
    // is interacting with
    void selectAll();

    absl::optional<int> encodingMib() const;

    // Get the text description of the encoding effectively used,
    // suitable to display to the user.
    QString encodingText() const;

    // Returns whether follow is enabled in this crawler
    bool isFollowEnabled() const;

    // Set content of search line
    void setSearchLineEditText( const QString& text );

    // Get content of search line
    QString currentSearchLineEditText() const;

  public slots:
    // Stop the asynchoronous loading of the file if one is in progress
    // The file is identified by the view attached to it.
    void stopLoading();
    // Reload the displayed file
    void reload();
    // Set the encoding
    void setEncoding( absl::optional<int> mib );

    void focusSearchEdit();

    // Instructs the widget to reconfigure itself because Config() has changed.
    void applyConfiguration();

  public:
    template <class T> struct access_by;

  protected:
    // Implementation of the ViewInterface functions
    void doSetData( std::shared_ptr<LogData> log_data,
                    std::shared_ptr<LogFilteredData> filtered_data ) override;
    void doSetQuickFindPattern( std::shared_ptr<QuickFindPattern> qfp ) override;
    void doSetSavedSearches( SavedSearches* saved_searches ) override;
    void doSetViewContext( const QString& view_context ) override;
    std::shared_ptr<const ViewContextInterface> doGetViewContext( void ) const override;

    // Implementation of the mux selector interface
    // (for dispatching QuickFind to the right widget)
    SearchableWidgetInterface* doGetActiveSearchable() const override;
    std::vector<QObject*> doGetAllSearchables() const override;

    // Implementation of the MuxableDocumentInterface
    void doSendAllStateSignals() override;

    void keyPressEvent( QKeyEvent* keyEvent ) override;
    void changeEvent( QEvent* event ) override;

  signals:
    // Sent to signal the client load has progressed,
    // passing the completion percentage.
    void loadingProgressed( int progress );
    // Sent to the client when the loading has finished
    // weither succesfull or not.
    void loadingFinished( LoadingStatus status );
    // Sent when follow mode is enabled/disabled
    void followSet( bool checked );
    // Sent up to the MainWindow to enable/disable the follow mode
    void followModeChanged( bool follow );
    // Sent up when the current line number is updated
    void updateLineNumber( LineNumber line );

    // "auto-refresh" check has been changed
    void searchRefreshChanged( bool isRefreshing );
    // "ignore case" check has been changed
    void matchCaseChanged( bool matchCase );

    // Sent when the data status (whether new not seen data are
    // available) has changed
    void dataStatusChanged( DataStatus status );

  private slots:
    // Instructs the widget to start a search using the current search line.
    void startNewSearch();
    // Stop the currently ongoing search (if one exists)
    void stopSearch();
    void loadIcons();
    // QuickFind is being entered, save the focus for incremental qf.
    void enteringQuickFind();
    // QuickFind is being closed.
    void exitingQuickFind();
    // Called when new data must be displayed in the filtered window.
    void updateFilteredView( LinesCount nbMatches, int progress, LineNumber initialPosition );
    // Called when a new line has been selected in the filtered view,
    // to instruct the main view to jump to the matching line.
    void jumpToMatchingLine( LineNumber filteredLineNb );
    // Called when the main view is on a new line number
    void updateLineNumberHandler( LineNumber line );
    // Mark a line that has been clicked on the main (top) view.
    void markLinesFromMain( const std::vector<LineNumber>& lines );
    // Mark a line that has been clicked on the filtered (bottom) view.
    void markLinesFromFiltered( const std::vector<LineNumber>& lines );

    void loadingFinishedHandler( LoadingStatus status );
    // Manages the info lines to inform the user the file has changed.
    void fileChangedHandler( MonitoredFileStatus );

    void searchForward();
    void searchBackward();

    // Called when the checkbox for search auto-refresh is changed
    void searchRefreshChangedHandler( bool isRefreshing );

     // Called when the checkbox for case sensitivity is changed
    void matchCaseChangedHandler( bool shouldMatchCase );

    // Called when the text on the search line is modified
    void searchTextChangeHandler( QString );

    // Called when the user change the visibility combobox
    void changeFilteredViewVisibility( int index );

    // Called when the user add the string to the search
    void addToSearch( const QString& string );

    // Clear the search items
    void clearSearchItems();

    // Search Context Menu
    void showSearchContextMenu();

    // Called when a match is hovered on in the filtered view
    void mouseHoveredOverMatch( LineNumber line );

    // Called when there was activity in the views
    void activityDetected();

    void setSearchLimits( LineNumber startLine, LineNumber endLine );
    void clearSearchLimits();

  private:
    // State machine holding the state of the search, used to allow/disallow
    // auto-refresh and inform the user via the info line.
    class SearchState {
      public:
        enum State {
            NoSearch,
            Static,
            Autorefreshing,
            FileTruncated,
            TruncatedAutorefreshing,
        };

        SearchState()
        {
            state_ = NoSearch;
            autoRefreshRequested_ = false;
        }

        // Reset the state (no search active)
        void resetState();
        // The user changed auto-refresh request
        void setAutorefresh( bool refresh );
        // The file has been truncated (stops auto-refresh)
        void truncateFile();
        // The expression has been changed (stops auto-refresh)
        void changeExpression();
        // The search has been stopped (stops auto-refresh)
        void stopSearch();
        // The search has been started (enable auto-refresh)
        void startSearch();

        // Get the state in order to display the proper message
        State getState() const
        {
            return state_;
        }
        // Is auto-refresh allowed
        bool isAutorefreshAllowed() const
        {
            return ( state_ == Autorefreshing || state_ == TruncatedAutorefreshing );
        }
        bool isFileTruncated() const
        {
            return ( state_ == FileTruncated || state_ == TruncatedAutorefreshing );
        }

      private:
        State state_;
        bool autoRefreshRequested_;
    };

    // Private functions
    void setup();
    void replaceCurrentSearch( const QString& searchText );
    void updateSearchCombo();
    AbstractLogView* activeView() const;
    void printSearchInfoMessage( LinesCount nbMatches = 0_lcount );
    void changeDataStatus( DataStatus status );
    void updateEncoding();
    void changeTopViewSize( int32_t delta );

    // Palette for error notification (yellow background)
    static const QPalette errorPalette;

    LogMainView* logMainView;
    QWidget* bottomWindow;
    QLabel* searchLabel;
    QComboBox* searchLineEdit;
    QMenu* searchLineContextMenu;
    QCompleter* searchLineCompleter;
    QToolButton* searchButton;
    QToolButton* stopButton;
    FilteredView* filteredView;
    QComboBox* visibilityBox;
    InfoLine* searchInfoLine;
    QToolButton* matchCaseButton;
    QToolButton* useRegexpButton;
    QToolButton* searchRefreshButton;
    OverviewWidget* overviewWidget_;

    // Default palette to be remembered
    QPalette searchInfoLineDefaultPalette;

    SavedSearches* savedSearches_ = nullptr;

    // Reference to the QuickFind Pattern (not owned)
    std::shared_ptr<QuickFindPattern> quickFindPattern_;

    LogData* logData_ = nullptr;
    LogFilteredData* logFilteredData_ = nullptr;

    QWidget* qfSavedFocus_ = nullptr;

    // Search state (for auto-refresh and truncation)
    SearchState searchState_;

    // Matches overview
    Overview overview_;

    // Model for the visibility selector
    QStandardItemModel* visibilityModel_;

    // Last main line number received
    LineNumber currentLineNumber_;

    LineNumber searchStartLine_;
    LineNumber searchEndLine_;

    // Until we have received confirmation loading is finished, we
    // should consider we are loading something.
    bool loadingInProgress_ = true;

    // Is it not the first time we are loading something?
    bool firstLoadDone_ = false;

    // Saved marked lines to be restored on first load
    std::vector<LineNumber> savedMarkedLines_;

    // Current number of matches
    LinesCount nbMatches_;

    // the current dataStatus (whether we have new, not seen, data)
    DataStatus dataStatus_ = DataStatus::OLD_DATA;

    // Current encoding setting;
    absl::optional<int> encodingMib_;
    QString encoding_text_;

    IconLoader iconLoader_;
};

#endif
