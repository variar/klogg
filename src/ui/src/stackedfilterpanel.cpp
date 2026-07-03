/*
 * Copyright (C) 2026 klogg contributors
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

#include "stackedfilterpanel.h"

#include <limits>

#include <QCursor>
#include <QFont>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListView>
#include <QRegularExpression>
#include <QStandardItemModel>
#include <QVBoxLayout>

#include "configuration.h"
#include "infoline.h"
#include "logdata.h"
#include "predefinedfilters.h"
#include "regularexpression.h"
#include "regularexpressionpattern.h"

StackedFilterPanel::StackedFilterPanel( LogData* logData, QuickFindPattern* quickFindPattern,
                                        SavedSearches* savedSearches,
                                        const SearchResultArray& restrictToLines, QWidget* parent )
    : QWidget( parent )
    , logData_( logData )
    , savedSearches_( savedSearches )
    , iconLoader_( this )
{
    filteredData_ = logData_->getNewFilteredData();
    // Restrict this panel's search to the parent's results (sub-search).
    filteredData_->setSearchRestriction( restrictToLines );

    buildSearchBar();

    filteredView_ = new FilteredView( filteredData_.get(), quickFindPattern );
    filteredView_->setContentsMargins( 2, 0, 2, 0 );

    // Same widget order as CrawlerWidget's main search bar.
    auto* searchLineLayout = new QHBoxLayout;
    searchLineLayout->setContentsMargins( 2, 2, 2, 2 );
    searchLineLayout->addWidget( visibilityBox_ );
    searchLineLayout->addWidget( matchCaseButton_ );
    searchLineLayout->addWidget( useRegexpButton_ );
    searchLineLayout->addWidget( inverseButton_ );
    searchLineLayout->addWidget( booleanButton_ );
    searchLineLayout->addWidget( searchRefreshButton_ );
    searchLineLayout->addWidget( predefinedFilters_ );
    searchLineLayout->addWidget( searchLineEdit_ );
    searchLineLayout->addWidget( clearButton_ );
    searchLineLayout->addWidget( searchButton_ );
    searchLineLayout->addWidget( keepSearchResultsButton_ );
    searchLineLayout->addWidget( subSearchButton_ );
    searchLineLayout->addWidget( closeButton_ );
    searchLineLayout->addWidget( searchInfoLine_ );

    auto* layout = new QVBoxLayout;
    layout->setContentsMargins( 2, 2, 2, 2 );
    layout->addLayout( searchLineLayout );
    layout->addWidget( filteredView_ );
    setLayout( layout );

    loadIcons();

    connect( searchLineEdit_->lineEdit(), &QLineEdit::returnPressed, searchButton_,
             &QToolButton::click );
    connect( searchButton_, &QToolButton::clicked, this, &StackedFilterPanel::startSearch );
    connect( clearButton_, &QToolButton::clicked, searchLineEdit_, &QComboBox::clearEditText );
    connect( subSearchButton_, &QToolButton::clicked, this,
             [ this ]() { Q_EMIT subSearchRequested( this ); } );
    connect( closeButton_, &QToolButton::clicked, this,
             [ this ]() { Q_EMIT closeRequested( this ); } );
    connect( visibilityBox_, QOverload<int>::of( &QComboBox::currentIndexChanged ), this,
             &StackedFilterPanel::changeVisibility );
    connect( predefinedFilters_, &PredefinedFiltersComboBox::filterChanged, this,
             &StackedFilterPanel::setSearchPatternFromPredefinedFilters );
    connect( searchLineEdit_, &QComboBox::customContextMenuRequested, this,
             &StackedFilterPanel::showSearchContextMenu );

    connect(
        filteredData_.get(), &LogFilteredData::searchProgressed, this,
        [ this ]( LinesCount nbMatches, int progress, LineNumber initialLine ) {
            filteredView_->updateData();
            if ( progress == 100 ) {
                searchInfoLine_->hideGauge();
                const QString text = nbMatches.get() > 1
                                         ? tr( "%1 matches found" ).arg( nbMatches.get() )
                                         : tr( "%1 match found" ).arg( nbMatches.get() );
                searchInfoLine_->setText( text );
                searchInfoLine_->setVisible( true );
            }
            else if ( progress > 0 ) {
                searchInfoLine_->setText(
                    tr( "Search in progress (%1 %)..." ).arg( QString::number( progress ) ) );
                searchInfoLine_->displayGauge( progress );
                searchInfoLine_->setVisible( true );
            }
            Q_EMIT searchProgressed( nbMatches, progress, initialLine );
            if ( progress == 100 ) {
                // Our results are final; let the owner cascade to the panel below.
                Q_EMIT resultsReady( this );
            }
        },
        Qt::QueuedConnection );
}

void StackedFilterPanel::rerunWithRestriction( const SearchResultArray& restrictToLines )
{
    filteredData_->setSearchRestriction( restrictToLines );
    startSearch();
}

void StackedFilterPanel::applyConfiguration()
{
    const auto& config = Configuration::get();
    QFont font = config.mainFont();

    // Match exactly how CrawlerWidget configures its filtered views.
    font.setKerning( false );
    font.setFixedPitch( true );
    if ( config.forceFontAntialiasing() ) {
        font.setStyleStrategy( QFont::PreferAntialias );
    }
    font.setBold( config.useBoldFont() );

    filteredView_->setLineNumbersVisible( config.filteredLineNumbersVisible() );
    filteredView_->allowFollowMode( config.anyFileWatchEnabled() );
    filteredView_->updateFont( font );

    // Search limits cover the whole file (no restriction to a sub-range). Without
    // this, lines are considered "outside the search limits" and drawn greyed-out
    // (Disabled text colour) instead of the normal foreground colour.
    filteredView_->setSearchLimits( 0_lnum, LineNumber( logData_->getNbLine().get() ) );
}

void StackedFilterPanel::buildSearchBar()
{
    const auto& config = Configuration::get();

    // Visibility combo (Marks and matches / Marks / Matches), same as main bar.
    using VisibilityFlags = LogFilteredData::VisibilityFlags;
    visibilityModel_ = new QStandardItemModel( this );

    auto* marksAndMatchesItem = new QStandardItem( tr( "Marks and matches" ) );
    marksAndMatchesItem->setData(
        QVariant::fromValue( VisibilityFlags::Marks | VisibilityFlags::Matches ) );
    visibilityModel_->appendRow( marksAndMatchesItem );

    auto* marksItem = new QStandardItem( tr( "Marks" ) );
    marksItem->setData( QVariant::fromValue<FilteredView::Visibility>( VisibilityFlags::Marks ) );
    visibilityModel_->appendRow( marksItem );

    auto* matchesItem = new QStandardItem( tr( "Matches" ) );
    matchesItem->setData(
        QVariant::fromValue<FilteredView::Visibility>( VisibilityFlags::Matches ) );
    visibilityModel_->appendRow( matchesItem );

    auto* visibilityView = new QListView( this );
    visibilityView->setMovement( QListView::Static );

    visibilityBox_ = new QComboBox();
    visibilityBox_->setModel( visibilityModel_ );
    visibilityBox_->setView( visibilityView );
    visibilityBox_->setCurrentIndex( 0 );
    visibilityBox_->setContentsMargins( 2, 2, 2, 2 );

    matchCaseButton_ = new QToolButton();
    matchCaseButton_->setToolTip( tr( "Match case" ) );
    matchCaseButton_->setCheckable( true );
    matchCaseButton_->setFocusPolicy( Qt::NoFocus );
    matchCaseButton_->setContentsMargins( 2, 2, 2, 2 );

    useRegexpButton_ = new QToolButton();
    useRegexpButton_->setToolTip( tr( "Use regex" ) );
    useRegexpButton_->setCheckable( true );
    useRegexpButton_->setFocusPolicy( Qt::NoFocus );
    useRegexpButton_->setContentsMargins( 2, 2, 2, 2 );

    inverseButton_ = new QToolButton();
    inverseButton_->setToolTip( tr( "Inverse match" ) );
    inverseButton_->setCheckable( true );
    inverseButton_->setFocusPolicy( Qt::NoFocus );
    inverseButton_->setContentsMargins( 2, 2, 2, 2 );

    booleanButton_ = new QToolButton();
    booleanButton_->setToolTip( tr( "Enable regular expression logical combining" ) );
    booleanButton_->setCheckable( true );
    booleanButton_->setFocusPolicy( Qt::NoFocus );
    booleanButton_->setContentsMargins( 2, 2, 2, 2 );

    searchRefreshButton_ = new QToolButton();
    searchRefreshButton_->setToolTip( tr( "Auto-refresh" ) );
    searchRefreshButton_->setCheckable( true );
    searchRefreshButton_->setFocusPolicy( Qt::NoFocus );
    searchRefreshButton_->setContentsMargins( 2, 2, 2, 2 );

    predefinedFilters_ = new PredefinedFiltersComboBox( this );
    predefinedFilters_->populatePredefinedFilters();

    searchLineCompleter_ = new QCompleter(
        savedSearches_ ? savedSearches_->recentSearches() : QStringList{}, this );
    searchLineEdit_ = new QComboBox;
    searchLineEdit_->setEditable( true );
    searchLineEdit_->setCompleter( searchLineCompleter_ );
    if ( savedSearches_ ) {
        searchLineEdit_->addItems( savedSearches_->recentSearches() );
    }
    searchLineEdit_->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Minimum );
    searchLineEdit_->setSizeAdjustPolicy( QComboBox::AdjustToMinimumContentsLengthWithIcon );
    searchLineEdit_->lineEdit()->setMaxLength( std::numeric_limits<int>::max() / 1024 );
    searchLineEdit_->lineEdit()->setPlaceholderText( tr( "Search within results above..." ) );
    searchLineEdit_->setContentsMargins( 2, 2, 2, 2 );

    searchLineContextMenu_ = searchLineEdit_->lineEdit()->createStandardContextMenu();
    searchLineEdit_->setContextMenuPolicy( Qt::CustomContextMenu );
    // Reset the edit text after construction (combo defaults to first history item).
    searchLineEdit_->clearEditText();

    setFocusProxy( searchLineEdit_ );

    clearButton_ = new QToolButton();
    clearButton_->setText( tr( "Clear search text" ) );
    clearButton_->setAutoRaise( true );
    clearButton_->setContentsMargins( 2, 2, 2, 2 );

    searchButton_ = new QToolButton();
    searchButton_->setText( tr( "Search" ) );
    searchButton_->setAutoRaise( true );
    searchButton_->setContentsMargins( 2, 2, 2, 2 );

    keepSearchResultsButton_ = new QToolButton();
    keepSearchResultsButton_->setText( tr( "Keep Results" ) );
    keepSearchResultsButton_->setToolTip(
        tr( "Keep these results and show subsequent results in a new window" ) );
    keepSearchResultsButton_->setCheckable( true );
    keepSearchResultsButton_->setContentsMargins( 2, 2, 2, 2 );

    subSearchButton_ = new QToolButton();
    subSearchButton_->setText( tr( "Sub-search" ) );
    subSearchButton_->setToolTip(
        tr( "Add a panel below that searches within these results (chainable)" ) );
    subSearchButton_->setContentsMargins( 2, 2, 2, 2 );

    closeButton_ = new QToolButton();
    closeButton_->setText( tr( "Close" ) );
    closeButton_->setToolTip( tr( "Close this panel and the ones below it" ) );
    closeButton_->setAutoRaise( true );
    closeButton_->setContentsMargins( 2, 2, 2, 2 );

    searchInfoLine_ = new InfoLine();
    searchInfoLine_->setFrameStyle( QFrame::StyledPanel );
    searchInfoLine_->setFrameShadow( QFrame::Sunken );
    searchInfoLine_->setLineWidth( 1 );
    searchInfoLine_->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum );
    searchInfoLine_->setContentsMargins( 2, 2, 2, 2 );

    // Same defaults as the main search bar.
    matchCaseButton_->setChecked( !config.isSearchIgnoreCaseDefault() );
    useRegexpButton_->setChecked( config.mainRegexpType() == SearchRegexpType::ExtendedRegexp );
    booleanButton_->setChecked( config.isSearchLogicalCombiningDefault() );
    searchRefreshButton_->setChecked( config.isSearchAutoRefreshDefault() );
}

void StackedFilterPanel::loadIcons()
{
    useRegexpButton_->setIcon( iconLoader_.load( "regex" ) );
    inverseButton_->setIcon( iconLoader_.load( "icons8-not-equal" ) );
    booleanButton_->setIcon( iconLoader_.load( "icons8-venn-diagram" ) );
    clearButton_->setIcon( iconLoader_.load( "icons8-delete" ) );
    searchButton_->setIcon( iconLoader_.load( "icons8-search" ) );
    matchCaseButton_->setIcon( iconLoader_.load( "icons8-font-size" ) );
    searchRefreshButton_->setIcon( iconLoader_.load( "icons8-search-refresh" ) );
    keepSearchResultsButton_->setIcon( iconLoader_.load( "icons8-lock" ) );
}

void StackedFilterPanel::setSearchPattern( const QString& pattern )
{
    searchLineEdit_->setEditText( pattern );
    startSearch();
}

QString StackedFilterPanel::escapeSearchPattern( const QString& pattern, bool isRegex ) const
{
    auto escapedPattern = ( !isRegex && useRegexpButton_->isChecked() )
                              ? QRegularExpression::escape( pattern )
                              : pattern;

    if ( booleanButton_->isChecked() ) {
        escapedPattern.replace( '"', "\"" ).prepend( '"' ).append( '"' );
    }

    return escapedPattern;
}

QString& StackedFilterPanel::combinePatterns( QString& currentPattern,
                                              const QString& newPattern ) const
{
    if ( !currentPattern.isEmpty() ) {
        if ( booleanButton_->isChecked() ) {
            currentPattern.append( " or " );
        }
        else if ( useRegexpButton_->isChecked() ) {
            currentPattern.append( '|' );
        }
    }

    currentPattern.append( newPattern );
    return currentPattern;
}

void StackedFilterPanel::setSearchPatternFromPredefinedFilters(
    const QList<PredefinedFilter>& filters )
{
    QString searchPattern;
    for ( const auto& filter : filters ) {
        combinePatterns( searchPattern, escapeSearchPattern( filter.pattern, filter.useRegex ) );
    }
    setSearchPattern( searchPattern );
}

void StackedFilterPanel::showSearchContextMenu()
{
    if ( searchLineContextMenu_ ) {
        searchLineContextMenu_->exec( QCursor::pos() );
    }
}

void StackedFilterPanel::setFocusToSearch()
{
    searchLineEdit_->setFocus();
}

void StackedFilterPanel::changeVisibility( int index )
{
    const auto visibility
        = visibilityModel_->item( index )->data().value<FilteredView::Visibility>();
    filteredView_->setVisibility( visibility );
    filteredView_->updateData();
}

void StackedFilterPanel::startSearch()
{
    const QString searchText = searchLineEdit_->currentText();

    filteredData_->clearSearch();
    filteredView_->updateData();

    if ( searchText.isEmpty() ) {
        searchInfoLine_->setText( {} );
        searchInfoLine_->setVisible( false );
        // No results here, but the panel below must still reflect the change.
        Q_EMIT resultsReady( this );
        return;
    }

    // Build the pattern from the same toggles as the main bar.
    auto regexpPattern = RegularExpressionPattern(
        searchText, matchCaseButton_->isChecked(), inverseButton_->isChecked(),
        booleanButton_->isChecked(), !useRegexpButton_->isChecked() );

    RegularExpression hsExpression{ regexpPattern };
    if ( !hsExpression.isValid() ) {
        filteredData_->clearSearch();
        filteredView_->updateData();
        searchInfoLine_->setText( tr( "Error in expression: " ) + hsExpression.errorString() );
        searchInfoLine_->setVisible( true );
        filteredView_->setSearchPattern( {} );
        Q_EMIT resultsReady( this );
        return;
    }

    filteredData_->runSearch( regexpPattern, 0_lnum, LineNumber( logData_->getNbLine().get() ) );
    filteredView_->setSearchPattern( regexpPattern );
}
