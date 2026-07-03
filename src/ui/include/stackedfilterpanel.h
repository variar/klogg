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

#ifndef STACKEDFILTERPANEL_H
#define STACKEDFILTERPANEL_H

#include <memory>

#include <QComboBox>
#include <QCompleter>
#include <QMenu>
#include <QString>
#include <QToolButton>
#include <QWidget>

#include "filteredview.h"
#include "iconloader.h"
#include "logfiltereddata.h"
#include "predefinedfilterscombobox.h"
#include "savedsearches.h"

class LogData;
class QuickFindPattern;
class InfoLine;
class QStandardItemModel;

// A self-contained stacked sub-search panel. Its search bar is built to look
// and behave exactly like the main CrawlerWidget search bar (same buttons,
// icons and toggles), and its FilteredView renders identically. Several of
// these can be stacked below the main view to chain searches (search within
// the results of the panel above).
//
// The panel owns its own LogFilteredData (built from the shared LogData) and
// is seeded with a restriction set (the matching lines of the panel above it),
// so its search only matches within those lines.
class StackedFilterPanel : public QWidget {
    Q_OBJECT

  public:
    StackedFilterPanel( LogData* logData, QuickFindPattern* quickFindPattern,
                        SavedSearches* savedSearches, const SearchResultArray& restrictToLines,
                        QWidget* parent = nullptr );

    FilteredView* filteredView() const
    {
        return filteredView_;
    }

    LogFilteredData* filteredData() const
    {
        return filteredData_.get();
    }

    std::shared_ptr<LogFilteredData> filteredDataPtr() const
    {
        return filteredData_;
    }

    void setFocusToSearch();

    // Replace the restriction set (parent's results) and re-run the currently
    // entered search term against it. Used to cascade changes down the chain.
    void rerunWithRestriction( const SearchResultArray& restrictToLines );

    // Apply the global configuration (font, line numbers, follow mode) to this
    // panel's view, so it looks exactly like the main filtered view.
    void applyConfiguration();

  Q_SIGNALS:
    // User pressed the "Sub-search" button: a deeper panel should be created
    // below this one, restricted to this panel's results.
    void subSearchRequested( StackedFilterPanel* source );
    // User pressed the close button: this panel (and everything below it)
    // should be removed.
    void closeRequested( StackedFilterPanel* source );
    // The search finished or progressed; lets the owner update info text.
    void searchProgressed( LinesCount nbMatches, int progress, LineNumber initialLine );
    // Emitted when this panel's search has fully completed (100%). The owner
    // uses this to cascade the recompute to the panel below.
    void resultsReady( StackedFilterPanel* panel );

  private Q_SLOTS:
    void startSearch();
    void changeVisibility( int index );
    void setSearchPatternFromPredefinedFilters( const QList<PredefinedFilter>& filters );
    void showSearchContextMenu();

  private:
    void buildSearchBar();
    void loadIcons();
    void setSearchPattern( const QString& pattern );
    QString escapeSearchPattern( const QString& pattern, bool isRegex ) const;
    QString& combinePatterns( QString& currentPattern, const QString& newPattern ) const;

    LogData* logData_;
    std::shared_ptr<LogFilteredData> filteredData_;
    SavedSearches* savedSearches_;
    IconLoader iconLoader_;

    // Search bar widgets (mirroring CrawlerWidget's bar)
    QComboBox* visibilityBox_;
    QStandardItemModel* visibilityModel_;
    QToolButton* matchCaseButton_;
    QToolButton* useRegexpButton_;
    QToolButton* inverseButton_;
    QToolButton* booleanButton_;
    QToolButton* searchRefreshButton_;
    PredefinedFiltersComboBox* predefinedFilters_;
    QComboBox* searchLineEdit_;
    QCompleter* searchLineCompleter_;
    QMenu* searchLineContextMenu_;
    QToolButton* clearButton_;
    QToolButton* searchButton_;
    QToolButton* keepSearchResultsButton_;
    QToolButton* subSearchButton_;
    QToolButton* closeButton_;
    InfoLine* searchInfoLine_;

    FilteredView* filteredView_;
};

#endif // STACKEDFILTERPANEL_H
