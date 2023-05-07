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

#include <cstddef>
#include <optional>

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QSplitter>
#include <QToolButton>
#include <QVBoxLayout>

#include "colorlabelsmanager.h"
#include "loadingstatus.h"
#include "logdata.h"
#include "logfiltereddata.h"
#include "filteredview.h"
#include "iconloader.h"
#include "logmainview.h"
#include "overview.h"
#include "predefinedfilterscombobox.h"
#include "signalmux.h"
#include "viewinterface.h"

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

    std::optional<int> encodingMib() const;

    // Get the text description of the encoding effectively used,
    // suitable to display to the user.
    QString encodingText() const;

    // Returns whether follow is enabled in this crawler
    bool isFollowEnabled() const;

    void registerShortcuts();

  public Q_SLOTS:
    // Stop the asynchoronous loading of the file if one is in progress
    // The file is identified by the view attached to it.
    void stopLoading();
    // Reload the displayed file
    void reload();
    // Set the encoding
    void setEncoding( std::optional<int> mib );

    void focusSearchEdit();
    void goToLine();

    // Instructs the widget to reconfigure itself because Config() has changed.
    void applyConfiguration();

  public:
    template <class T>
    struct access_by;

  protected:
    // Implementation of the ViewInterface functions
    void doSetData( std::shared_ptr<LogData> logData,
                    std::shared_ptr<LogFilteredData> filteredData ) override;
    void doSetQuickFindPattern( std::shared_ptr<QuickFindPattern> qfp ) override;
    void doSetSavedSearches( SavedSearches* savedSearches ) override;
    void doSetViewContext( const QString& viewContext ) override;
    std::shared_ptr<const ViewContextInterface> doGetViewContext( void ) const override;

    // Implementation of the mux selector interface
    // (for dispatching QuickFind to the right widget)
    SearchableWidgetInterface* doGetActiveSearchable() const override;
    std::vector<QObject*> doGetAllSearchables() const override;

    // Implementation of the MuxableDocumentInterface
    void doSendAllStateSignals() override;

    void changeEvent( QEvent* event ) override;

  Q_SIGNALS:
    // Sent to signal the client load has progressed,
    // passing the completion percentage.
    void loadingProgressed( int progress );
    // Sent to the client when the loading has finished
    // whether successful or not.
    void loadingFinished( LoadingStatus status );
    // Sent when follow mode is enabled/disabled
    void followSet( bool checked );
    // Sent up to the MainWindow to enable/disable the follow mode
    void followModeChanged( bool follow );
    // Sent up when the current line number is updated
    void updateLineNumber( LineNumber line );
    // Sent up when user wants to save new predefined filter from current search
    void saveCurrentSearchAsPredefinedFilter( QString newFilter );

    void sendToScratchpad(QString);
    void replaceDataInScratchpad(QString);

    // "auto-refresh" check has been changed
    void searchRefreshChanged( bool isRefreshing );
    // "ignore case" check has been changed
    void matchCaseChanged( bool matchCase );

    // Sent when the data status (whether new not seen data are
    // available) has changed
    void dataStatusChanged( DataStatus status );

  private Q_SLOTS:
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

    // Called when the checkbox for boolean combining is changed
    void booleanCombiningChangedHandler( bool shouldCombine );

    // Called when the checkbox for using regex is changed
    void useRegexpChangeHandler( bool shouldUseRegex );

    // Called when the text on the search line is modified
    void searchTextChangeHandler( QString );

    // Called when the user change the visibility combobox
    void changeFilteredViewVisibility( int index );

    // Called when the user add the string to the search
    void addToSearch( const QString& string );

    // Called when the user replaces the search with the selection string
    void replaceSearch( const QString& string );

    // Called when the user excludes selection string from search
    // works only in boolean combination mode
    void excludeFromSearch( const QString& string );

    void clearSearchHistory();
    void editSearchHistory();

    // Save current search as predefined filter
    void saveAsPredefinedFilter();
    void setSearchPatternFromPredefinedFilters( const QList<PredefinedFilter>& filters );

    // Search Context Menu
    void showSearchContextMenu();

    // Called when a match is hovered on in the filtered view
    void mouseHoveredOverMatch( LineNumber line );

    // Called when there was activity in the views
    void activityDetected();

    void setSearchLimits( LineNumber startLine, LineNumber endLine );
    void clearSearchLimits();

    void addColorLabelToSelection( size_t label );
    void addNextColorLabelToSelection();
    void clearColorLabels();

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
    void setShortcuts();
    void replaceCurrentSearch( const QString& searchText );
    void updateSearchCombo();
    AbstractLogView* activeView() const;
    void printSearchInfoMessage( LinesCount nbMatches = 0_lcount );
    void changeDataStatus( DataStatus status );
    void updateEncoding();
    void changeTopViewSize( int32_t delta );
    void updatePredefinedFiltersWidget();

    // Reload predefined filters after changing settings
    void reloadPredefinedFilters() const;

    QString escapeSearchPattern( const QString& searchPattern, bool isRegex = false ) const;
    QString& combinePatterns( QString& currentPattern, const QString& newPattern ) const;
    void setSearchPattern( const QString& searchPattern );

    void resetStateOnSearchPatternChanges();

    void updateColorLabels( const ColorLabelsManager::QuickHighlightersCollection& labels );

    // Palette for error notification (yellow background)
    static const QPalette ErrorPalette;

    IconLoader iconLoader_;

    SavedSearches* savedSearches_ = nullptr;

    std::shared_ptr<LogData> logData_;
    std::shared_ptr<LogFilteredData> logFilteredData_;

    // Matches overview
    Overview overview_;

    std::shared_ptr<QuickFindPattern> quickFindPattern_;

    LogMainView* logMainView_;
    FilteredView* filteredView_;

    OverviewWidget* overviewWidget_;

    QComboBox* visibilityBox_;
    QStandardItemModel* visibilityModel_;

    PredefinedFiltersComboBox* predefinedFilters_;

    QComboBox* searchLineEdit_;
    QMenu* searchLineContextMenu_;
    QCompleter* searchLineCompleter_;

    InfoLine* searchInfoLine_;

    QToolButton* clearButton_;
    QToolButton* searchButton_;
    QToolButton* stopButton_;

    QToolButton* matchCaseButton_;
    QToolButton* useRegexpButton_;
    QToolButton* inverseButton_;
    QToolButton* booleanButton_;
    QToolButton* searchRefreshButton_;

    std::map<QString, QShortcut*> shortcuts_;

    // Default palette to be remembered
    QPalette searchInfoLineDefaultPalette_;

    // Reference to the QuickFind Pattern (not owned)

    QWidget* qfSavedFocus_ = nullptr;

    // Search state (for auto-refresh and truncation)
    SearchState searchState_;

    // the current dataStatus (whether we have new, not seen, data)
    DataStatus dataStatus_ = DataStatus::OLD_DATA;

    // Last main line number received
    LineNumber currentLineNumber_;

    // Current number of matches
    LinesCount nbMatches_;

    LineNumber searchStartLine_;
    LineNumber searchEndLine_;

    // Until we have received confirmation loading is finished, we
    // should consider we are loading something.
    bool loadingInProgress_ = true;
    bool firstLoadDone_ = false;

    std::vector<LineNumber> savedMarkedLines_;

    // Current encoding setting;
    std::optional<int> encodingMib_;
    QString encodingText_;

    ColorLabelsManager colorLabelsManager_;
};

#endif
