/*
 * Copyright (C) 2009, 2010, 2011, 2012, 2013, 2014, 2015 Nicolas Bonnefon and other contributors
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

// This file implements the CrawlerWidget class.
// It is responsible for creating and managing the two views and all
// the UI elements.  It implements the connection between the UI elements.
// It also interacts with the sets of data (full and filtered).

#include "log.h"

#include <cassert>
#include <chrono>

#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QHeaderView>
#include <QJsonDocument>
#include <QLineEdit>
#include <QListView>
#include <QStandardItemModel>
#include <Qt>

#include "crawlerwidget.h"

#include "configuration.h"
#include "infoline.h"
#include "overview.h"
#include "quickfindpattern.h"
#include "quickfindwidget.h"
#include "savedsearches.h"

// Palette for error signaling (yellow background)
const QPalette CrawlerWidget::errorPalette( QColor( "yellow" ) );

// Implementation of the view context for the CrawlerWidget
class CrawlerWidgetContext : public ViewContextInterface {
  public:
    // Construct from the stored string representation
    explicit CrawlerWidgetContext( const QString& string );
    // Construct from the value passsed
    CrawlerWidgetContext( QList<int> sizes, bool ignore_case, bool auto_refresh, bool follow_file, bool use_regexp,
                          QList<LineNumber> markedLines )
        : sizes_( sizes )
        , ignore_case_( ignore_case )
        , auto_refresh_( auto_refresh )
        , follow_file_( follow_file )
        , use_regexp_( use_regexp )
    {
        std::transform( markedLines.begin(), markedLines.end(), std::back_inserter( marks_ ),
                        []( const auto& m ) { return m.get(); } );
    }

    // Implementation of the ViewContextInterface function
    QString toString() const override;

    // Access the Qt sizes array for the QSplitter
    QList<int> sizes() const
    {
        return sizes_;
    }

    bool ignoreCase() const
    {
        return ignore_case_;
    }
    bool autoRefresh() const
    {
        return auto_refresh_;
    }
    bool followFile() const
    {
        return follow_file_;
    }
    bool useRegexp() const
    {
        return use_regexp_;
    }

    QList<LineNumber::UnderlyingType> marks() const
    {
        return marks_;
    }

  private:
    void loadFromString( const QString& string );
    void loadFromJson( const QString& json );

  private:
    QList<int> sizes_;

    bool ignore_case_;
    bool auto_refresh_;
    bool follow_file_;
    bool use_regexp_;

    QList<LineNumber::UnderlyingType> marks_;
};

// Constructor only does trivial construction. The real work is done once
// the data is attached.
CrawlerWidget::CrawlerWidget( QWidget* parent )
    : QSplitter( parent )
    , overview_()
{
    logData_ = nullptr;
    logFilteredData_ = nullptr;

    quickFindPattern_ = nullptr;
    savedSearches_ = nullptr;
    qfSavedFocus_ = nullptr;

    // Until we have received confirmation loading is finished, we
    // should consider we are loading something.
    loadingInProgress_ = true;
    // and it's the first time
    firstLoadDone_ = false;
    dataStatus_ = DataStatus::OLD_DATA;
}

// The top line is first one on the main display
LineNumber CrawlerWidget::getTopLine() const
{
    return logMainView->getTopLine();
}

QString CrawlerWidget::getSelectedText() const
{
    if ( filteredView->hasFocus() )
        return filteredView->getSelection();
    else
        return logMainView->getSelection();
}

bool CrawlerWidget::isPartialSelection() const
{
    if ( filteredView->hasFocus() )
        return filteredView->isPartialSelection();
    else
        return logMainView->isPartialSelection();
}

void CrawlerWidget::selectAll()
{
    activeView()->selectAll();
}

CrawlerWidget::Encoding CrawlerWidget::encodingSetting() const
{
    return encodingSetting_;
}

bool CrawlerWidget::isFollowEnabled() const
{
    return logMainView->isFollowEnabled();
}

QString CrawlerWidget::encodingText() const
{
    return encoding_text_;
}

// Return a pointer to the view in which we should do the QuickFind
SearchableWidgetInterface* CrawlerWidget::doGetActiveSearchable() const
{
    return activeView();
}

// Return all the searchable widgets (views)
std::vector<QObject*> CrawlerWidget::doGetAllSearchables() const
{
    std::vector<QObject*> searchables = { logMainView, filteredView };

    return searchables;
}

// Update the state of the parent
void CrawlerWidget::doSendAllStateSignals()
{
    emit updateLineNumber( currentLineNumber_ );
    if ( !loadingInProgress_ )
        emit loadingFinished( LoadingStatus::Successful );
}

void CrawlerWidget::keyPressEvent( QKeyEvent* keyEvent )
{
    const auto noModifier = keyEvent->modifiers() == Qt::NoModifier;

    if ( keyEvent->key() == Qt::Key_V && noModifier )
        visibilityBox->setCurrentIndex( ( visibilityBox->currentIndex() + 1 )
                                        % visibilityBox->count() );
    else {
        switch ( keyEvent->key() ) {
        case Qt::Key_Plus:
            changeTopViewSize( 1 );
            break;
        case Qt::Key_Minus:
            changeTopViewSize( -1 );
            break;
        default:
            QSplitter::keyPressEvent( keyEvent );
            break;
        }
    }
}

//
// Public slots
//

void CrawlerWidget::stopLoading()
{
    logFilteredData_->interruptSearch();
    logData_->interruptLoading();
}

void CrawlerWidget::reload()
{
    searchState_.resetState();
    logFilteredData_->clearSearch();
    logFilteredData_->clearMarks();
    filteredView->updateData();
    printSearchInfoMessage();

    logData_->reload();

    // A reload is considered as a first load,
    // this is to prevent the "new data" icon to be triggered.
    firstLoadDone_ = false;
}

void CrawlerWidget::setEncoding( Encoding encoding )
{
    encodingSetting_ = encoding;
    updateEncoding();

    update();
}

//
// Protected functions
//
void CrawlerWidget::doSetData( std::shared_ptr<LogData> log_data,
                               std::shared_ptr<LogFilteredData> filtered_data )
{
    logData_ = log_data.get();
    logFilteredData_ = filtered_data.get();
}

void CrawlerWidget::doSetQuickFindPattern( std::shared_ptr<QuickFindPattern> qfp )
{
    quickFindPattern_ = qfp;
}

void CrawlerWidget::doSetSavedSearches( SavedSearches* saved_searches )
{
    savedSearches_ = saved_searches;

    // We do setup now, assuming doSetData has been called before
    // us, that's not great really...
    setup();
}

void CrawlerWidget::doSetViewContext( const QString& view_context )
{
    LOG( logDEBUG ) << "CrawlerWidget::doSetViewContext: " << view_context.toLocal8Bit().data();

    const auto context = CrawlerWidgetContext{ view_context };

    setSizes( context.sizes() );
    matchCaseButton->setChecked( !context.ignoreCase() );
    useRegexpButton->setChecked( context.useRegexp() );

    auto auto_refresh_check_state = context.autoRefresh() ? Qt::Checked : Qt::Unchecked;
    searchRefreshCheck->setCheckState( auto_refresh_check_state );
    // Manually call the handler as it is not called when changing the state programmatically
    searchRefreshChangedHandler( auto_refresh_check_state );

    logMainView->followSet( context.followFile() );

    const auto savedMarks = context.marks();
    std::transform( savedMarks.begin(), savedMarks.end(), std::back_inserter( savedMarkedLines_ ),
                    []( const auto& l ) { return LineNumber( l ); } );
}

std::shared_ptr<const ViewContextInterface> CrawlerWidget::doGetViewContext() const
{
    auto context = std::make_shared<const CrawlerWidgetContext>(
        sizes(),
        ( !matchCaseButton->isChecked() ),
        ( searchRefreshCheck->checkState() == Qt::Checked ),
        logMainView->isFollowEnabled(),
        useRegexpButton->isChecked(),
        logFilteredData_->getMarks() );

    return static_cast<std::shared_ptr<const ViewContextInterface>>( context );
}

//
// Slots
//

void CrawlerWidget::startNewSearch()
{
    // Record the search line in the recent list
    // (reload the list first in case another glogg changed it)
    auto &searches = Persistable::getSynced<SavedSearches>();
    savedSearches_->addRecent( searchLineEdit->currentText() );
    searches.save();

    // Update the SearchLine (history)
    updateSearchCombo();
    // Call the private function to do the search
    replaceCurrentSearch( searchLineEdit->currentText() );
}

void CrawlerWidget::stopSearch()
{
    logFilteredData_->interruptSearch();
    searchState_.stopSearch();
    printSearchInfoMessage();
}

// When receiving the 'newDataAvailable' signal from LogFilteredData
void CrawlerWidget::updateFilteredView( LinesCount nbMatches, int progress,
                                        LineNumber initialPosition )
{
    LOG( logDEBUG ) << "updateFilteredView received.";

    searchInfoLine->show();

    if ( progress == 100 ) {
        // Searching done
        printSearchInfoMessage( nbMatches );
        searchInfoLine->hideGauge();
        // De-activate the stop button
        stopButton->setEnabled( false );
    }
    else {
        // Search in progress
        // We ignore 0% and 100% to avoid a flash when the search is very short
        if ( progress > 0 ) {
            searchInfoLine->setText( tr( "Search in progress (%1 %)... %2 match%3 found so far." )
                                         .arg( QString::number( progress ),
                                               QString::number( nbMatches.get() ),
                                               QLatin1String( nbMatches.get() > 1 ? "es" : "" ) ) );

            searchInfoLine->displayGauge( progress );
        }
    }

    static auto lastUpdateTime = std::chrono::high_resolution_clock::now();

    const auto currentUpdateTime = std::chrono::high_resolution_clock::now();
    const auto timeSinceLastUpdate = currentUpdateTime - lastUpdateTime;
    if ( progress > 0 && progress < 100
         && timeSinceLastUpdate < std::chrono::milliseconds( 250 ) ) {
        LOG( logDEBUG ) << "updateFilteredView skipped";
        return;
    }
    lastUpdateTime = currentUpdateTime;

    // If more (or less, e.g. come back to 0) matches have been found
    if ( nbMatches != nbMatches_ ) {
        nbMatches_ = nbMatches;

        // Recompute the content of the filtered window.
        filteredView->updateData();

        // Update the match overview
        overview_.updateData( logData_->getNbLine() );

        // New data found icon
        if ( initialPosition > 0_lnum ) {
            changeDataStatus( DataStatus::NEW_FILTERED_DATA );
        }

        // Also update the top window for the coloured bullets.
        update();
    }

    // Try to restore the filtered window selection close to where it was
    // only for full searches to avoid disconnecting follow mode!
    if ( ( progress == 100 ) && ( initialPosition == 0_lnum ) && ( !isFollowEnabled() ) ) {
        const auto currenLineIndex = logFilteredData_->getLineIndexNumber( currentLineNumber_ );
        LOG( logDEBUG ) << "updateFilteredView: restoring selection: "
                        << " absolute line number (0based) " << currentLineNumber_ << " index "
                        << currenLineIndex;
        filteredView->selectAndDisplayLine( currenLineIndex );
        filteredView->setSearchLimits( searchStartLine_, searchEndLine_ );
    }
}

void CrawlerWidget::jumpToMatchingLine( LineNumber filteredLineNb )
{
    const auto mainViewLine = logFilteredData_->getMatchingLineNumber( filteredLineNb );
    logMainView->selectAndDisplayLine( mainViewLine ); // FIXME: should be done with a signal.
}

void CrawlerWidget::updateLineNumberHandler( LineNumber line )
{
    currentLineNumber_ = line;
    emit updateLineNumber( line );
}

void CrawlerWidget::markLineFromMain( LineNumber line )
{
    if ( line < logData_->getNbLine() ) {
        if ( logFilteredData_->isLineMarked( line ) )
            logFilteredData_->deleteMark( line );
        else
            logFilteredData_->addMark( line );

        // Recompute the content of both window.
        filteredView->updateData();
        logMainView->updateData();

        // Update the match overview
        overview_.updateData( logData_->getNbLine() );

        // Also update the top window for the coloured bullets.
        update();
    }
}

void CrawlerWidget::markLineFromFiltered( LineNumber line )
{
    if ( line < logFilteredData_->getNbLine() ) {
        const auto line_in_file = logFilteredData_->getMatchingLineNumber( line );
        if ( logFilteredData_->filteredLineTypeByIndex( line ).testFlag( LogFilteredData::FilteredLineTypeFlags::Mark ) )
            logFilteredData_->deleteMark( line_in_file );
        else
            logFilteredData_->addMark( line_in_file );

        // Recompute the content of both window.
        filteredView->updateData();
        logMainView->updateData();

        // Update the match overview
        overview_.updateData( logData_->getNbLine() );

        // Also update the top window for the coloured bullets.
        update();
    }
}

void CrawlerWidget::applyConfiguration()
{
    const auto& config = Persistable::get<Configuration>();
    QFont font = config.mainFont();

    LOG( logDEBUG ) << "CrawlerWidget::applyConfiguration";

    // Whatever font we use, we should NOT use kerning
    font.setKerning( false );
    font.setFixedPitch( true );
    // Necessary on systems doing subpixel positionning (e.g. Ubuntu 12.04)
    font.setStyleStrategy( QFont::ForceIntegerMetrics );

    logMainView->setFont( font );
    filteredView->setFont( font );

    logMainView->setLineNumbersVisible( config.mainLineNumbersVisible() );
    filteredView->setLineNumbersVisible( config.filteredLineNumbersVisible() );

    overview_.setVisible( config.isOverviewVisible() );
    logMainView->refreshOverview();

    logMainView->updateDisplaySize();
    logMainView->update();
    filteredView->updateDisplaySize();
    filteredView->update();

    // Update the SearchLine (history)
    updateSearchCombo();

    FileWatcher::getFileWatcher().setPolling();
}

void CrawlerWidget::enteringQuickFind()
{
    LOG( logDEBUG ) << "CrawlerWidget::enteringQuickFind";

    // Remember who had the focus (only if it is one of our views)
    QWidget* focus_widget = QApplication::focusWidget();

    if ( ( focus_widget == logMainView ) || ( focus_widget == filteredView ) )
        qfSavedFocus_ = focus_widget;
    else
        qfSavedFocus_ = nullptr;
}

void CrawlerWidget::exitingQuickFind()
{
    // Restore the focus once the QFBar has been hidden
    if ( qfSavedFocus_ )
        qfSavedFocus_->setFocus();
}

void CrawlerWidget::loadingFinishedHandler( LoadingStatus status )
{
    loadingInProgress_ = false;

    // We need to refresh the main window because the view lines on the
    // overview have probably changed.
    overview_.updateData( logData_->getNbLine() );

    // FIXME, handle topLine
    // logMainView->updateData( logData_, topLine );
    logMainView->updateData();

    // Shall we Forbid starting a search when loading in progress?
    // searchButton->setEnabled( false );

    // searchButton->setEnabled( true );

    // See if we need to auto-refresh the search
    if ( searchState_.isAutorefreshAllowed() ) {
        searchEndLine_ = LineNumber( logData_->getNbLine().get() );
        if ( searchState_.isFileTruncated() )
            // We need to restart the search
            replaceCurrentSearch( searchLineEdit->currentText() );
        else
            logFilteredData_->updateSearch( searchStartLine_, searchEndLine_ );
    }

    // Set the encoding for the views
    updateEncoding();

    clearSearchLimits();

    emit loadingFinished( status );

    // Also change the data available icon
    if ( firstLoadDone_ ) {
        changeDataStatus( DataStatus::NEW_DATA );
    }
    else {
        firstLoadDone_ = true;
        for ( const auto& m : savedMarkedLines_ ) {
            logFilteredData_->addMark( m );
        }
    }
}

void CrawlerWidget::fileChangedHandler( LogData::MonitoredFileStatus status )
{
    // Handle the case where the file has been truncated
    if ( status == LogData::Truncated ) {
        // Clear all marks (TODO offer the option to keep them)
        logFilteredData_->clearMarks();
        if ( !searchInfoLine->text().isEmpty() ) {
            // Invalidate the search
            logFilteredData_->clearSearch();
            filteredView->updateData();
            searchState_.truncateFile();
            printSearchInfoMessage();
            nbMatches_ = 0_lcount;
        }
    }
}

// Returns a pointer to the window in which the search should be done
AbstractLogView* CrawlerWidget::activeView() const
{
    QWidget* activeView;

    // Search in the window that has focus, or the window where 'Find' was
    // called from, or the main window.
    if ( filteredView->hasFocus() || logMainView->hasFocus() )
        activeView = QApplication::focusWidget();
    else
        activeView = qfSavedFocus_;

    if ( activeView ) {
        auto* view = qobject_cast<AbstractLogView*>( activeView );
        return view;
    }
    else {
        LOG( logWARNING ) << "No active view, defaulting to logMainView";
        return logMainView;
    }
}

void CrawlerWidget::searchForward()
{
    LOG( logDEBUG ) << "CrawlerWidget::searchForward";

    activeView()->searchForward();
}

void CrawlerWidget::searchBackward()
{
    LOG( logDEBUG ) << "CrawlerWidget::searchBackward";

    activeView()->searchBackward();
}

void CrawlerWidget::searchRefreshChangedHandler( int state )
{
    searchState_.setAutorefresh( state == Qt::Checked );
    printSearchInfoMessage( logFilteredData_->getNbMatches() );
}

void CrawlerWidget::searchTextChangeHandler( QString )
{
    // We suspend auto-refresh
    searchState_.changeExpression();
    printSearchInfoMessage( logFilteredData_->getNbMatches() );
}

void CrawlerWidget::changeFilteredViewVisibility( int index )
{
    QStandardItem* item = visibilityModel_->item( index );
    FilteredView::Visibility visibility
        = static_cast<FilteredView::Visibility>( item->data().toInt() );

    filteredView->setVisibility( visibility );

    if ( logFilteredData_->getNbLine() > 0_lcount ) {
        const auto lineIndex = logFilteredData_->getLineIndexNumber( currentLineNumber_ );
        filteredView->selectAndDisplayLine( lineIndex );
    }
}

void CrawlerWidget::addToSearch( const QString& string )
{
    QString text = searchLineEdit->currentText();

    if ( text.isEmpty() )
        text = string;
    else {
        // Escape the regexp chars from the string before adding it.
        text += ( '|' + QRegularExpression::escape( string ) );
    }

    searchLineEdit->setEditText( text );

    // Set the focus to lineEdit so that the user can press 'Return' immediately
    searchLineEdit->lineEdit()->setFocus();
}

void CrawlerWidget::mouseHoveredOverMatch( LineNumber line )
{
    const auto line_in_mainview = logFilteredData_->getMatchingLineNumber( line );

    overviewWidget_->highlightLine( line_in_mainview );
}

void CrawlerWidget::activityDetected()
{
    changeDataStatus( DataStatus::OLD_DATA );
}

void CrawlerWidget::setSearchLimits( LineNumber startLine, LineNumber endLine )
{
    searchStartLine_ = startLine;
    searchEndLine_ = endLine;

    logMainView->setSearchLimits( startLine, endLine );
    filteredView->setSearchLimits( startLine, endLine );
}

void CrawlerWidget::clearSearchLimits()
{
    setSearchLimits( 0_lnum, LineNumber( logData_->getNbLine().get() ) );
}

//
// Private functions
//

// Build the widget and connect all the signals, this must be done once
// the data are attached.
void CrawlerWidget::setup()
{
    setOrientation( Qt::Vertical );

    assert( logData_ );
    assert( logFilteredData_ );

    // The views
    bottomWindow = new QWidget;
    overviewWidget_ = new OverviewWidget();
    logMainView = new LogMainView( logData_, quickFindPattern_.get(), &overview_, overviewWidget_ );
    filteredView = new FilteredView( logFilteredData_, quickFindPattern_.get() );

    overviewWidget_->setOverview( &overview_ );
    overviewWidget_->setParent( logMainView );

    // Connect the search to the top view
    logMainView->useNewFiltering( logFilteredData_ );

    // Construct the visibility button
    visibilityModel_ = new QStandardItemModel( this );

    QStandardItem* marksAndMatchesItem = new QStandardItem( tr( "Marks and matches" ) );
    QPixmap marksAndMatchesPixmap( 16, 10 );
    marksAndMatchesPixmap.fill( Qt::gray );
    marksAndMatchesItem->setIcon( QIcon( marksAndMatchesPixmap ) );
    marksAndMatchesItem->setData( QVariant::fromValue( FilteredView::Visibility::MarksAndMatches ) );
    visibilityModel_->appendRow( marksAndMatchesItem );

    QStandardItem* marksItem = new QStandardItem( tr( "Marks" ) );
    QPixmap marksPixmap( 16, 10 );
    marksPixmap.fill( Qt::blue );
    marksItem->setIcon( QIcon( marksPixmap ) );
    marksItem->setData( QVariant::fromValue( FilteredView::Visibility::MarksOnly ) );
    visibilityModel_->appendRow( marksItem );

    QStandardItem* matchesItem = new QStandardItem( tr( "Matches" ) );
    QPixmap matchesPixmap( 16, 10 );
    matchesPixmap.fill( Qt::red );
    matchesItem->setIcon( QIcon( matchesPixmap ) );
    matchesItem->setData( QVariant::fromValue( FilteredView::Visibility::MatchesOnly ) );
    visibilityModel_->appendRow( matchesItem );

    auto* visibilityView = new QListView( this );
    visibilityView->setMovement( QListView::Static );
    visibilityView->setMinimumWidth( 170 ); // Only needed with custom style-sheet

    visibilityBox = new QComboBox();
    visibilityBox->setModel( visibilityModel_ );
    visibilityBox->setView( visibilityView );

    // Select "Marks and matches" by default (same default as the filtered view)
    visibilityBox->setCurrentIndex( 0 );

    // TODO: Maybe there is some way to set the popup width to be
    // sized-to-content (as it is when the stylesheet is not overriden) in the
    // stylesheet as opposed to setting a hard min-width on the view above.
    visibilityBox->setStyleSheet( " \
        QComboBox:on {\
            padding: 1px 2px 1px 6px;\
            width: 19px;\
        } \
        QComboBox:!on {\
            padding: 1px 2px 1px 7px;\
            width: 19px;\
            height: 16px;\
            border: 1px solid gray;\
        } \
        QComboBox::drop-down::down-arrow {\
            width: 0px;\
            border-width: 0px;\
        } \
" );

    // Construct the Search Info line
    searchInfoLine = new InfoLine();
    searchInfoLine->setFrameStyle( QFrame::WinPanel | QFrame::Sunken );
    searchInfoLine->setLineWidth( 1 );
    auto searchInfoLineSizePolicy = searchInfoLine->sizePolicy();
    searchInfoLineSizePolicy.setRetainSizeWhenHidden( true );
    searchInfoLine->setSizePolicy( searchInfoLineSizePolicy );
    searchInfoLineDefaultPalette = searchInfoLine->palette();

    matchCaseButton = new QPushButton();
    matchCaseButton->setToolTip( "Match case" );
    matchCaseButton->setIcon( QIcon( ":/images/case48.png" ) );
    matchCaseButton->setCheckable( true );
    matchCaseButton->setFocusPolicy( Qt::NoFocus );

    useRegexpButton = new QPushButton();
    useRegexpButton->setToolTip( "Use regex" );
    useRegexpButton->setIcon( QIcon( ":/images/regex.png" ) );
    useRegexpButton->setCheckable( true );
    useRegexpButton->setFocusPolicy( Qt::NoFocus );

    searchRefreshCheck = new QCheckBox( "Auto-&refresh" );

    // Construct the Search line
    searchLabel = new QLabel( tr( "&Text: " ) );
    searchLineEdit = new QComboBox;
    searchLineEdit->setEditable( true );
    searchLineEdit->setCompleter( nullptr );
    searchLineEdit->addItems( savedSearches_->recentSearches() );
    searchLineEdit->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Minimum );
    searchLineEdit->setSizeAdjustPolicy( QComboBox::AdjustToMinimumContentsLengthWithIcon );
    searchLineEdit->lineEdit()->setMaxLength( std::numeric_limits<int>::max() / 1024 );

    searchLabel->setBuddy( searchLineEdit );

    setFocusProxy( searchLineEdit );

    searchButton = new QToolButton();
    searchButton->setText( tr( "&Search" ) );
    searchButton->setAutoRaise( true );

    stopButton = new QToolButton();
    stopButton->setIcon( QIcon( ":/images/stop14.png" ) );
    stopButton->setAutoRaise( true );
    stopButton->setEnabled( false );

    auto* searchLineLayout = new QHBoxLayout;
    searchLineLayout->addWidget( matchCaseButton );
    searchLineLayout->addWidget( useRegexpButton );
    //searchLineLayout->addWidget( searchLabel );
    searchLineLayout->addWidget( searchLineEdit );
    searchLineLayout->addWidget( searchButton );
    searchLineLayout->addWidget( stopButton );
    searchLineLayout->setContentsMargins( 6, 0, 6, 0 );
    stopButton->setSizePolicy( QSizePolicy( QSizePolicy::Maximum, QSizePolicy::Maximum ) );
    searchButton->setSizePolicy( QSizePolicy( QSizePolicy::Maximum, QSizePolicy::Maximum ) );

    auto* searchInfoLineLayout = new QHBoxLayout;
    searchInfoLineLayout->addWidget( visibilityBox );
    searchInfoLineLayout->addWidget( searchInfoLine );

    searchInfoLineLayout->addWidget( searchRefreshCheck );

    // Construct the bottom window
    auto* bottomMainLayout = new QVBoxLayout;
    bottomMainLayout->addLayout( searchLineLayout );
    bottomMainLayout->addLayout( searchInfoLineLayout );
    bottomMainLayout->addWidget( filteredView );
    bottomMainLayout->setContentsMargins( 2, 1, 2, 1 );
    bottomWindow->setLayout( bottomMainLayout );

    addWidget( logMainView );
    addWidget( bottomWindow );

    // Default search checkboxes
    auto& config = Persistable::get<Configuration>();
    searchRefreshCheck->setCheckState( config.isSearchAutoRefreshDefault() ? Qt::Checked
                                                                           : Qt::Unchecked );
    // Manually call the handler as it is not called when changing the state programmatically
    searchRefreshChangedHandler( searchRefreshCheck->checkState() );
    matchCaseButton->setChecked( config.isSearchIgnoreCaseDefault() ? false : true );

    useRegexpButton->setChecked( config.mainRegexpType() == ExtendedRegexp );

    // Default splitter position (usually overridden by the config file)
    setSizes( config.splitterSizes() );

    // Connect the signals
    connect( searchLineEdit->lineEdit(), &QLineEdit::returnPressed, searchButton,
             &QToolButton::click );
    connect( searchLineEdit->lineEdit(), &QLineEdit::textEdited, this,
             &CrawlerWidget::searchTextChangeHandler );
    connect( searchButton, &QToolButton::clicked, this, &CrawlerWidget::startNewSearch );
    connect( stopButton, &QToolButton::clicked, this, &CrawlerWidget::stopSearch );

    connect( visibilityBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this,
             &CrawlerWidget::changeFilteredViewVisibility );

    connect( logMainView, &LogMainView::newSelection, [this]( auto ) { logMainView->update(); } );
    connect( filteredView, &FilteredView::newSelection,
             [this]( auto ) { filteredView->update(); } );

    connect( filteredView, &FilteredView::newSelection, this, &CrawlerWidget::jumpToMatchingLine );

    connect( logMainView, &LogMainView::updateLineNumber, this,
             &CrawlerWidget::updateLineNumberHandler );
    connect( logMainView, &LogMainView::markLine, this, &CrawlerWidget::markLineFromMain );
    connect( filteredView, &FilteredView::markLine, this, &CrawlerWidget::markLineFromFiltered );

    connect( logMainView, QOverload<const QString&>::of( &LogMainView::addToSearch ), this,
             &CrawlerWidget::addToSearch );
    connect( filteredView, QOverload<const QString&>::of( &FilteredView::addToSearch ), this,
             &CrawlerWidget::addToSearch );

    connect( filteredView, &FilteredView::mouseHoveredOverLine, this,
             &CrawlerWidget::mouseHoveredOverMatch );
    connect( filteredView, &FilteredView::mouseLeftHoveringZone, overviewWidget_,
             &OverviewWidget::removeHighlight );

    // Follow option (up and down)
    connect( this, &CrawlerWidget::followSet, logMainView, &LogMainView::followSet );
    connect( this, &CrawlerWidget::followSet, filteredView, &FilteredView::followSet );
    connect( logMainView, &LogMainView::followModeChanged, this,
             &CrawlerWidget::followModeChanged );
    connect( filteredView, &FilteredView::followModeChanged, this,
             &CrawlerWidget::followModeChanged );

    // Detect activity in the views
    connect( logMainView, &LogMainView::activity, this, &CrawlerWidget::activityDetected );
    connect( filteredView, &FilteredView::activity, this, &CrawlerWidget::activityDetected );

    connect( logMainView, &LogMainView::changeSearchLimits, this, &CrawlerWidget::setSearchLimits );
    connect( filteredView, &FilteredView::changeSearchLimits, this,
             &CrawlerWidget::setSearchLimits );

    connect( logMainView, &LogMainView::clearSearchLimits, this,
             &CrawlerWidget::clearSearchLimits );
    connect( filteredView, &FilteredView::clearSearchLimits, this,
             &CrawlerWidget::clearSearchLimits );

    auto saveSplitterSizes = [this, &config]() { config.setSplitterSizes( this->sizes() ); };

    connect( logMainView, &LogMainView::saveDefaultSplitterSizes, saveSplitterSizes );
    connect( filteredView, &FilteredView::saveDefaultSplitterSizes, saveSplitterSizes );

    connect( logFilteredData_, &LogFilteredData::searchProgressed, this,
             &CrawlerWidget::updateFilteredView, Qt::QueuedConnection );

    // Sent load file update to MainWindow (for status update)
    connect( logData_, &LogData::loadingProgressed, this, &CrawlerWidget::loadingProgressed );
    connect( logData_, &LogData::loadingFinished, this, &CrawlerWidget::loadingFinishedHandler );
    connect( logData_, &LogData::fileChanged, this, &CrawlerWidget::fileChangedHandler );

    // Search auto-refresh
    connect( searchRefreshCheck, &QCheckBox::stateChanged, this,
             &CrawlerWidget::searchRefreshChangedHandler );

    // Advise the parent the checkboxes have been changed
    // (for maintaining default config)
    connect( searchRefreshCheck, &QCheckBox::stateChanged, this,
             &CrawlerWidget::searchRefreshChanged );
    connect( matchCaseButton, &QPushButton::toggled, this, &CrawlerWidget::matchCaseChanged );

    // Switch between views
    connect( logMainView, &LogMainView::exitView, filteredView,
             QOverload<>::of( &FilteredView::setFocus ) );
    connect( filteredView, &FilteredView::exitView, logMainView,
             QOverload<>::of( &LogMainView::setFocus ) );
}

// Create a new search using the text passed, replace the currently
// used one and destroy the old one.
void CrawlerWidget::replaceCurrentSearch( const QString& searchText )
{
    // Interrupt the search if it's ongoing
    logFilteredData_->interruptSearch();

    // We have to wait for the last search update (100%)
    // before clearing/restarting to avoid having remaining results.

    // FIXME: this is a bit of a hack, we call processEvents
    // for Qt to empty its event queue, including (hopefully)
    // the search update event sent by logFilteredData_. It saves
    // us the overhead of having proper sync.
    QApplication::processEvents( QEventLoop::ExcludeUserInputEvents );

    nbMatches_ = 0_lcount;

    // Clear and recompute the content of the filtered window.
    logFilteredData_->clearSearch();
    filteredView->updateData();

    // Update the match overview
    overview_.updateData( logData_->getNbLine() );

    if ( !searchText.isEmpty() ) {

        QString pattern;

        if ( !useRegexpButton->isChecked() ) {
            pattern = QRegularExpression::escape( searchText );
        }
        else {
            pattern = searchText;
        }

        // Set the pattern case insensitive if needed
        QRegularExpression::PatternOptions patternOptions
            = QRegularExpression::UseUnicodePropertiesOption;

        if ( !matchCaseButton->isChecked() )
            patternOptions |= QRegularExpression::CaseInsensitiveOption;

        // Constructs the regexp
        QRegularExpression regexp( pattern, patternOptions );

        if ( regexp.isValid() ) {
            // Activate the stop button
            stopButton->setEnabled( true );
            // Start a new asynchronous search
            logFilteredData_->runSearch( regexp, searchStartLine_, searchEndLine_ );
            // Accept auto-refresh of the search
            searchState_.startSearch();
            searchInfoLine->hide();
        }
        else {
            // The regexp is wrong
            logFilteredData_->clearSearch();
            filteredView->updateData();
            searchState_.resetState();

            // Inform the user
            QString errorMessage = tr( "Error in expression" );
            const int offset = regexp.patternErrorOffset();
            if ( offset != -1 ) {
                errorMessage += " at position ";
                errorMessage += QString::number( offset );
            }
            errorMessage += ": ";
            errorMessage += regexp.errorString();
            searchInfoLine->setPalette( errorPalette );
            searchInfoLine->setText( errorMessage );
            searchInfoLine->show();
        }
    }
    else {
        searchState_.resetState();
        printSearchInfoMessage();
    }
}

// Updates the content of the drop down list for the saved searches,
// called when the SavedSearch has been changed.
void CrawlerWidget::updateSearchCombo()
{
    const QString text = searchLineEdit->lineEdit()->text();
    searchLineEdit->clear();
    searchLineEdit->addItems( savedSearches_->recentSearches() );
    // In case we had something that wasn't added to the list (blank...):
    searchLineEdit->lineEdit()->setText( text );
}

// Print the search info message.
void CrawlerWidget::printSearchInfoMessage( LinesCount nbMatches )
{
    QString text;

    switch ( searchState_.getState() ) {
    case SearchState::NoSearch:
        // Blank text is fine
        break;
    case SearchState::Static:
        text = tr( "%1 match%2 found." )
                   .arg( nbMatches.get() )
                   .arg( nbMatches.get() > 1 ? "es" : "" );
        break;
    case SearchState::Autorefreshing:
        text = tr( "%1 match%2 found. Search is auto-refreshing..." )
                   .arg( nbMatches.get() )
                   .arg( nbMatches.get() > 1 ? "es" : "" );
        break;
    case SearchState::FileTruncated:
    case SearchState::TruncatedAutorefreshing:
        text = tr( "File truncated on disk, previous search results are not valid anymore." );
        break;
    }

    searchInfoLine->setPalette( searchInfoLineDefaultPalette );
    searchInfoLine->setText( text );
    searchInfoLine->setVisible( ! text.isEmpty() );
}

// Change the data status and, if needed, advise upstream.
void CrawlerWidget::changeDataStatus( DataStatus status )
{
    if ( ( status != dataStatus_ )
         && ( !( dataStatus_ == DataStatus::NEW_FILTERED_DATA
                 && status == DataStatus::NEW_DATA ) ) ) {
        dataStatus_ = status;
        emit dataStatusChanged( dataStatus_ );
    }
}

// Determine the right encoding and set the views.
void CrawlerWidget::updateEncoding()
{
    QTextCodec* textCodec = QTextCodec::codecForName( "iso-8859-1" );
    switch ( encodingSetting_ ) {
    case Encoding::AUTO:
        textCodec = logData_->getDetectedEncoding();
        encoding_text_ = tr( textCodec->name().constData() );
        break;
    case Encoding::UTF8:
        textCodec = QTextCodec::codecForName( "utf-8" );
        break;
    case Encoding::CP1251:
        textCodec = QTextCodec::codecForName( "windows-1251" );
        break;
    case Encoding::UTF16LE:
        textCodec = QTextCodec::codecForName( "utf-16le" );
        break;
    case Encoding::UTF16BE:
        textCodec = QTextCodec::codecForName( "utf-16be" );
        break;
    case Encoding::UTF32LE:
        textCodec = QTextCodec::codecForName( "utf-32le" );
        break;
    case Encoding::UTF32BE:
        textCodec = QTextCodec::codecForName( "utf-32be" );
        break;
    case Encoding::BIG5:
        textCodec = QTextCodec::codecForName( "big5" );
        break;
    case Encoding::GB18030:
        textCodec = QTextCodec::codecForName( "gb18030" );
        break;
    case Encoding::SHIFT_JIS:
        textCodec = QTextCodec::codecForName( "shift-jis" );
        break;
    case Encoding::KOI8R:
        textCodec = QTextCodec::codecForName( "koi8-r" );
        break;
    case Encoding::LOCAL:
        textCodec = QTextCodec::codecForLocale();
        break;
    case Encoding::ISO_8859_1:
    default:
        break;
    }

    if ( encodingSetting_ != Encoding::AUTO ) {
        QString displayedAs( "Displayed as %1" );
        encoding_text_ = tr( displayedAs.arg( textCodec->name().constData() ).toLatin1() );
    }

    logData_->setDisplayEncoding( textCodec->name().constData() );
    logMainView->forceRefresh();
    logFilteredData_->setDisplayEncoding( textCodec->name().constData() );
    filteredView->forceRefresh();
}

// Change the respective size of the two views
void CrawlerWidget::changeTopViewSize( int32_t delta )
{
    int min, max;
    getRange( 1, &min, &max );
    LOG( logDEBUG ) << "CrawlerWidget::changeTopViewSize " << sizes().at( 0 ) << " " << min << " "
                    << max;
    moveSplitter( closestLegalPosition( sizes().at( 0 ) + ( delta * 10 ), 1 ), 1 );
    LOG( logDEBUG ) << "CrawlerWidget::changeTopViewSize " << sizes().at( 0 );
}

//
// SearchState implementation
//
void CrawlerWidget::SearchState::resetState()
{
    state_ = NoSearch;
}

void CrawlerWidget::SearchState::setAutorefresh( bool refresh )
{
    autoRefreshRequested_ = refresh;

    if ( refresh ) {
        if ( state_ == Static )
            state_ = Autorefreshing;
        /*
        else if ( state_ == FileTruncated )
            state_ = TruncatedAutorefreshing;
        */
    }
    else {
        if ( state_ == Autorefreshing )
            state_ = Static;
        else if ( state_ == TruncatedAutorefreshing )
            state_ = FileTruncated;
    }
}

void CrawlerWidget::SearchState::truncateFile()
{
    if ( state_ == Autorefreshing || state_ == TruncatedAutorefreshing ) {
        state_ = TruncatedAutorefreshing;
    }
    else {
        state_ = FileTruncated;
    }
}

void CrawlerWidget::SearchState::changeExpression()
{
    if ( state_ == Autorefreshing )
        state_ = Static;
}

void CrawlerWidget::SearchState::stopSearch()
{
    if ( state_ == Autorefreshing )
        state_ = Static;
}

void CrawlerWidget::SearchState::startSearch()
{
    if ( autoRefreshRequested_ )
        state_ = Autorefreshing;
    else
        state_ = Static;
}

/*
 * CrawlerWidgetContext
 */
CrawlerWidgetContext::CrawlerWidgetContext( const QString& string )
{
    if ( string.startsWith( '{' ) ) {
        loadFromJson( string );
    }
    else {
        loadFromString( string );
    }
}

void CrawlerWidgetContext::loadFromString( const QString& string )
{
    QRegularExpression regex( "S(\\d+):(\\d+)" );
    QRegularExpressionMatch match = regex.match( string );
    if ( match.hasMatch() ) {
        sizes_ = { match.captured( 1 ).toInt(), match.captured( 2 ).toInt() };
        LOG( logDEBUG ) << "sizes_: " << sizes_[ 0 ] << " " << sizes_[ 1 ];
    }
    else {
        LOG( logWARNING ) << "Unrecognised view size: " << string.toLocal8Bit().data();

        // Default values;
        sizes_ = { 400, 100 };
    }

    QRegularExpression case_refresh_regex( "IC(\\d+):AR(\\d+)" );
    match = case_refresh_regex.match( string );
    if ( match.hasMatch() ) {
        ignore_case_ = ( match.captured( 1 ).toInt() == 1 );
        auto_refresh_ = ( match.captured( 2 ).toInt() == 1 );

        LOG( logDEBUG ) << "ignore_case_: " << ignore_case_ << " auto_refresh_: " << auto_refresh_;
    }
    else {
        LOG( logWARNING ) << "Unrecognised case/refresh: " << string.toLocal8Bit().data();
        ignore_case_ = false;
        auto_refresh_ = false;
    }

    QRegularExpression follow_regex( "AR(\\d+):FF(\\d+)" );
    match = follow_regex.match( string );
    if ( match.hasMatch() ) {
        follow_file_ = ( match.captured( 2 ).toInt() == 1 );

        LOG( logDEBUG ) << "follow_file_: " << follow_file_;
    }
    else {
        LOG( logWARNING ) << "Unrecognised follow file " << string.toLocal8Bit().data();
        follow_file_ = false;
    }

    use_regexp_ = Persistable::get<Configuration>().mainRegexpType() == ExtendedRegexp;
}

void CrawlerWidgetContext::loadFromJson( const QString& json )
{
    const auto properties = QJsonDocument::fromJson( json.toLatin1() ).toVariant().toMap();

    if ( properties.contains( "S" ) ) {
        const auto sizes = properties.value( "S" ).toList();
        for ( const auto& s : sizes ) {
            sizes_.append( s.toInt() );
        }
    }

    ignore_case_ = properties.value( "IC" ).toBool();
    auto_refresh_ = properties.value( "AR" ).toBool();
    follow_file_ = properties.value( "FF" ).toBool();
    if ( properties.contains( "RE" ) ) {
        use_regexp_ = properties.value( "RE" ).toBool();
    }
    else {
        use_regexp_ = Persistable::get<Configuration>().mainRegexpType() == ExtendedRegexp;
    }

    if ( properties.contains( "M" ) ) {
        const auto marks = properties.value( "M" ).toList();
        for ( const auto& m : marks ) {
            marks_.append( m.toUInt() );
        }
    }
}

QString CrawlerWidgetContext::toString() const
{
    const auto toVariantList = []( const auto& list ) -> QVariantList {
        QVariantList variantList;
        for ( const auto& item : list ) {
            variantList.append( item );
        }
        return variantList;
    };

    QVariantMap properies;

    properies[ "S" ] = toVariantList( sizes_ );
    properies[ "IC" ] = ignore_case_;
    properies[ "AR" ] = auto_refresh_;
    properies[ "FF" ] = follow_file_;
    properies[ "RE" ] = use_regexp_;
    properies[ "M" ] = toVariantList( marks_ );

    return QJsonDocument::fromVariant( properies ).toJson( QJsonDocument::Compact );
}
