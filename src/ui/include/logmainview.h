/*
 * Copyright (C) 2009, 2010, 2011, 2013 Nicolas Bonnefon
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

#ifndef LOGMAINVIEW_H
#define LOGMAINVIEW_H

#include <memory>

#include "abstractlogview.h"
#include "logdata.h"
#include "logfiltereddata.h"

class LogMainViewData : public AbstractLogData
{
  Q_OBJECT

  public:
    explicit LogMainViewData( const LogData* sourceLogData );

    void setTopFilter( const RegularExpressionPattern& pattern );
    void prepareFullRefresh();
    void refreshSearch();
    void interruptSearch();

    LineNumber getSourceLineNumber( LineNumber index ) const;
    LineNumber getLineIndexNumber( LineNumber sourceLine ) const;
    LinesCount getNbTotalLines() const;

  Q_SIGNALS:
    void searchProgressed( LinesCount nbMatches, int progress, LineNumber initialLine );

  protected:
    QString doGetLineString( LineNumber line ) const override;
    QString doGetExpandedLineString( LineNumber line ) const override;
    klogg::vector<QString> doGetLines( LineNumber firstLine, LinesCount number ) const override;
    klogg::vector<QString> doGetExpandedLines( LineNumber firstLine,
                                               LinesCount number ) const override;
    LineNumber doGetLineNumber( LineNumber index ) const override;
    LinesCount doGetNbLine() const override;
    LineLength doGetMaxLength() const override;
    LineLength doGetLineLength( LineNumber line ) const override;
    void doSetDisplayEncoding( const char* encoding ) override;
    QTextCodec* doGetDisplayEncoding() const override;
    void doAttachReader() const override;
    void doDetachReader() const override;

  private:
    const AbstractLogData* activeData() const;

    const LogData* sourceLogData_;
    std::unique_ptr<LogFilteredData> filteredData_;
    RegularExpressionPattern filterPattern_;
    bool filterEnabled_ = false;
    bool fullRefreshRequired_ = false;
};

// Class implementing the main (top) view widget.
class LogMainView : public AbstractLogView
{
  Q_OBJECT
  public:
    LogMainView( LogMainViewData* newLogData,
            const QuickFindPattern* const quickFindPattern,
            Overview* overview,
            OverviewWidget* overview_widget,
            QWidget* parent = nullptr );

    LineNumber getTopSourceLine() const;
    void trySelectSourceLine( LineNumber sourceLine );
    void selectSourcePortionAndDisplayLine( LineNumber sourceLine, LinesCount nLines,
                                            LineColumn startCol, LineLength nSymbols );

    // Configure the view to use the passed filtered list
    // (used for couloured bullets)
    // Should be NULL or the empty LFD if no filtering is used
    void useNewFiltering( LogFilteredData* filteredData );

  protected:
    // Implements the virtual function
    LogData::LineType lineType( LineNumber lineNumber ) const override;

    LineNumber displayLineNumber( LineNumber lineNumber ) const override;
    LineNumber lineIndex( LineNumber lineNumber ) const override;
    LineNumber maxDisplayLineNumber() const override;

    void doRegisterShortcuts() override;

  private:
    LogFilteredData* filteredData_;
    LogMainViewData* logMainViewData_;
};

#endif
