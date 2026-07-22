/*
 * Copyright (C) 2009, 2010, 2011, 2013, 2017 Nicolas Bonnefon
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

// This file implements the LogMainView concrete class.
// Most of the actual drawing and event management is done in AbstractLogView
// Only behaviour specific to the main (top) view is implemented here.

#include "logmainview.h"

#include "abstractlogdata.h"
#include "log.h"
#include "logfiltereddata.h"
#include "overview.h"

#include "shortcuts.h"

LogMainViewData::LogMainViewData( const LogData* sourceLogData )
    : sourceLogData_( sourceLogData )
    , filteredData_( sourceLogData->getNewFilteredData() )
{
    connect( filteredData_.get(), &LogFilteredData::searchProgressed, this,
             &LogMainViewData::searchProgressed );
}

void LogMainViewData::setTopFilter( const RegularExpressionPattern& pattern )
{
    const auto enableFilter = !pattern.pattern.isEmpty();
    if ( filterEnabled_ == enableFilter && filterPattern_.pattern == pattern.pattern
         && filterPattern_.isCaseSensitive == pattern.isCaseSensitive ) {
        return;
    }

    filteredData_->interruptSearch();
    filterPattern_ = pattern;
    filterEnabled_ = enableFilter;
    fullRefreshRequired_ = false;

    if ( filterEnabled_ ) {
        filteredData_->runSearch( filterPattern_ );
    }
    else {
        filteredData_->clearSearch();
    }
}

void LogMainViewData::prepareFullRefresh()
{
    if ( filterEnabled_ ) {
        constexpr auto DropCache = true;
        filteredData_->clearSearch( DropCache );
        fullRefreshRequired_ = true;
    }
}

void LogMainViewData::refreshSearch()
{
    if ( !filterEnabled_ ) {
        return;
    }

    if ( fullRefreshRequired_ ) {
        fullRefreshRequired_ = false;
        filteredData_->runSearch( filterPattern_ );
    }
    else {
        filteredData_->updateSearch( 0_lnum, LineNumber( sourceLogData_->getNbLine().get() ) );
    }
}

void LogMainViewData::interruptSearch()
{
    filteredData_->interruptSearch();
}

LineNumber LogMainViewData::getSourceLineNumber( LineNumber index ) const
{
    return filterEnabled_ ? filteredData_->getMatchingLineNumber( index ) : index;
}

LineNumber LogMainViewData::getLineIndexNumber( LineNumber sourceLine ) const
{
    return filterEnabled_ ? filteredData_->getLineIndexNumber( sourceLine ) : sourceLine;
}

LinesCount LogMainViewData::getNbTotalLines() const
{
    return sourceLogData_->getNbLine();
}

const AbstractLogData* LogMainViewData::activeData() const
{
    return filterEnabled_ ? static_cast<const AbstractLogData*>( filteredData_.get() )
                          : static_cast<const AbstractLogData*>( sourceLogData_ );
}

QString LogMainViewData::doGetLineString( LineNumber line ) const
{
    return activeData()->getLineString( line );
}

QString LogMainViewData::doGetExpandedLineString( LineNumber line ) const
{
    return activeData()->getExpandedLineString( line );
}

klogg::vector<QString> LogMainViewData::doGetLines( LineNumber firstLine,
                                                    LinesCount number ) const
{
    return activeData()->getLines( firstLine, number );
}

klogg::vector<QString> LogMainViewData::doGetExpandedLines( LineNumber firstLine,
                                                            LinesCount number ) const
{
    return activeData()->getExpandedLines( firstLine, number );
}

LineNumber LogMainViewData::doGetLineNumber( LineNumber index ) const
{
    return getSourceLineNumber( index );
}

LinesCount LogMainViewData::doGetNbLine() const
{
    return activeData()->getNbLine();
}

LineLength LogMainViewData::doGetMaxLength() const
{
    return activeData()->getMaxLength();
}

LineLength LogMainViewData::doGetLineLength( LineNumber line ) const
{
    return activeData()->getLineLength( line );
}

void LogMainViewData::doSetDisplayEncoding( const char* encoding )
{
    Q_UNUSED( encoding )
}

QTextCodec* LogMainViewData::doGetDisplayEncoding() const
{
    return sourceLogData_->getDisplayEncoding();
}

void LogMainViewData::doAttachReader() const
{
    sourceLogData_->attachReader();
}

void LogMainViewData::doDetachReader() const
{
    sourceLogData_->detachReader();
}

LogMainView::LogMainView( LogMainViewData* newLogData,
                          const QuickFindPattern* const quickFindPattern,
                          Overview* overview, OverviewWidget* overview_widget, QWidget* parent )
    : AbstractLogView( newLogData, quickFindPattern, parent )
    , logMainViewData_( newLogData )
{
    filteredData_ = nullptr;

    // The main data has a real (non NULL) Overview
    setOverview( overview, overview_widget );
}

LineNumber LogMainView::getTopSourceLine() const
{
    return logMainViewData_->getSourceLineNumber( getTopLine() );
}

void LogMainView::trySelectSourceLine( LineNumber sourceLine )
{
    if ( logMainViewData_->getNbLine() > 0_lcount ) {
        trySelectLine( logMainViewData_->getLineIndexNumber( sourceLine ) );
    }
}

void LogMainView::selectSourcePortionAndDisplayLine( LineNumber sourceLine, LinesCount nLines,
                                                     LineColumn startCol, LineLength nSymbols )
{
    if ( logMainViewData_->getNbLine() > 0_lcount ) {
        selectPortionAndDisplayLine( logMainViewData_->getLineIndexNumber( sourceLine ), nLines,
                                     startCol, nSymbols );
    }
}

// Just update our internal record.
void LogMainView::useNewFiltering( LogFilteredData* filteredData )
{
    filteredData_ = filteredData;

    if ( getOverview() != nullptr )
        getOverview()->setFilteredData( filteredData_ );

    forceRefresh();
}

AbstractLogData::LineType LogMainView::lineType( LineNumber lineNumber ) const
{
    if ( filteredData_ ) {
        return filteredData_->lineTypeByLine(
            logMainViewData_->getSourceLineNumber( lineNumber ) );
    }
    return AbstractLogData::LineTypeFlags::Plain;
}

LineNumber LogMainView::displayLineNumber( LineNumber lineNumber ) const
{
    return logMainViewData_->getSourceLineNumber( lineNumber ) + 1_lcount;
}

LineNumber LogMainView::lineIndex( LineNumber lineNumber ) const
{
    return logMainViewData_->getLineIndexNumber( lineNumber );
}

LineNumber LogMainView::maxDisplayLineNumber() const
{
    return LineNumber( logMainViewData_->getNbTotalLines().get() );
}

void LogMainView::doRegisterShortcuts()
{
    LOG_INFO << "Registering shortcuts for main view";
    AbstractLogView::doRegisterShortcuts();
    registerShortcut( ShortcutAction::LogViewNextMark, [ this ] {
        const auto line = filteredData_->getMarkAfter(
            logMainViewData_->getSourceLineNumber( getViewPosition() ) );
        if ( line.has_value() ) {
            trySelectSourceLine( *line );
        }
    } );
    registerShortcut( ShortcutAction::LogViewPrevMark, [ this ] {
        const auto line = filteredData_->getMarkBefore(
            logMainViewData_->getSourceLineNumber( getViewPosition() ) );
        if ( line.has_value() ) {
            trySelectSourceLine( *line );
        }
    } );
}
