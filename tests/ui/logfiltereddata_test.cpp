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
 
#include <catch2/catch.hpp>

#include <QSignalSpy>
#include <QTemporaryFile>
#include <QTest>
#include <QTimer>

#include "configuration.h"
#include "log.h"
#include "test_utils.h"

#include "logdata.h"
#include "logfiltereddata.h"

static const qint64 SL_NB_LINES = 500LL;

namespace {

bool generateDataFiles( QTemporaryFile& file )
{
    char newLine[ 90 ];

    if ( file.open() ) {
        for ( int i = 0; i < SL_NB_LINES; i++ ) {
            snprintf( newLine, 89,
                      "LOGDATA \t is a part of glogg, we are going to test it thoroughly, this is "
                      "line %06d\n",
                      i );
            file.write( newLine, (qint64) qstrlen( newLine ) );
        }
        file.flush();
    }

    return true;
}

void runSearch( LogFilteredData* filtered_data, const QString& regexp,
                SafeQSignalSpy& searchProgressSpy )
{

    QTimer::singleShot(
        50, [ & ]() { filtered_data->runSearch( RegularExpressionPattern( regexp ) ); } );

    int progress = 0;
    do {
        REQUIRE( searchProgressSpy.wait() );
        QList<QVariant> progressArgs = searchProgressSpy.last();
        progress = progressArgs.at( 1 ).toInt();
    } while ( progress < 100 );
}

} // namespace

using LineTypeFlags = LogFilteredData::LineTypeFlags;
using VisibilityFlags = LogFilteredData::VisibilityFlags;
using LineType = LogFilteredData::LineType;

static LogFilteredData::LineTypeFlags toFlags( LogFilteredData::LineType type )
{
    return static_cast<LineTypeFlags>( static_cast<LineType::Int>( type ) );
}

struct LogDataLoader {
    LogDataLoader()
    {
        static int counter = 0;
        counter++;
        LOG_INFO << "Test run " << counter;

        REQUIRE( generateDataFiles( file ) );
        SafeQSignalSpy loadEndSpy( &log_data, SIGNAL( loadingFinished( LoadingStatus ) ) );

        log_data.attachFile( file.fileName() );
        REQUIRE( loadEndSpy.safeWait( 10000 ) );
    }

    QTemporaryFile file {"filtered_test_XXXXXX"};
    LogData log_data;
};

SCENARIO( "marks in filtered log data", "[logdata]" )
{
    LogDataLoader logDataLoader;

    GIVEN( "loaded log data" )
    {
        auto filtered_data = logDataLoader.log_data.getNewFilteredData();

        WHEN( "Adding mark outside file" )
        {
            filtered_data->addMark( LineNumber( SL_NB_LINES + 25 ) );

            THEN( "No marked lines stored" )
            {
                for ( LineNumber i = 0_lnum; i < LineNumber( SL_NB_LINES ); ++i )
                    REQUIRE_FALSE(
                        filtered_data->lineTypeByLine( i ).testFlag( LineTypeFlags::Mark ) );
            }
        }

        WHEN( "Adding marks in log file" )
        {
            filtered_data->addMark( 10_lnum );
            filtered_data->addMark( 25_lnum );

            AND_WHEN( "Check for marked line" )
            {
                THEN( "Return true" )
                {
                    REQUIRE(
                        filtered_data->lineTypeByLine( 10_lnum ).testFlag( LineTypeFlags::Mark ) );
                    REQUIRE(
                        filtered_data->lineTypeByLine( 25_lnum ).testFlag( LineTypeFlags::Mark ) );
                }
            }

            AND_WHEN( "Get marks count" )
            {
                THEN( "Return all marks count" )
                {
                    REQUIRE( filtered_data->getNbMarks() == 2_lcount );
                }

                AND_WHEN( "Get marks" )
                {
                    auto marks = filtered_data->getMarks();
                    THEN( "Provide all marks" )
                    {
                        REQUIRE( marks.size() == 2 );
                    }
                }
            }

            AND_WHEN( "Get mark before has mark" )
            {
                const auto markBefore = filtered_data->getMarkBefore( 25_lnum );
                THEN( "Return previous mark" )
                {
                    REQUIRE( markBefore.has_value() );
                    REQUIRE( *markBefore == 10_lnum );
                }
            }

            AND_WHEN( "Get mark before has no data" )
            {
                const auto markBefore = filtered_data->getMarkBefore( 10_lnum );
                THEN( "Return no mark" )
                {
                    REQUIRE_FALSE( markBefore.has_value() );
                }
            }

            AND_WHEN( "Get mark after has mark" )
            {
                const auto markAfter = filtered_data->getMarkAfter( 10_lnum );
                THEN( "Return next mark" )
                {
                    REQUIRE( markAfter.has_value() );
                    REQUIRE( *markAfter == 25_lnum );
                }
            }

            AND_WHEN( "Get mark after has no data" )
            {
                const auto markAfter = filtered_data->getMarkAfter( 25_lnum );
                THEN( "Return no mark" )
                {
                    REQUIRE_FALSE( markAfter.has_value() );
                }
            }

            AND_WHEN( "Delete mark" )
            {
                filtered_data->deleteMark( 10_lnum );
                THEN( "Mark is removed" )
                {
                    REQUIRE_FALSE(
                        filtered_data->lineTypeByLine( 10_lnum ).testFlag( LineTypeFlags::Mark ) );
                    REQUIRE( filtered_data->getNbMarks() == 1_lcount );
                }
            }

            AND_WHEN( "Clear marks" )
            {
                filtered_data->clearMarks();
                THEN( "All marks are removed" )
                {
                    REQUIRE( filtered_data->getNbMarks() == 0_lcount );
                }
            }
        }
    }
}

SCENARIO( "search for regex", "[logdata]" )
{
    LogDataLoader logDataLoader;

    GIVEN( "loaded log data" )
    {
        auto filtered_data = logDataLoader.log_data.getNewFilteredData();

        WHEN( "Searched for regex" )
        {
            const auto threadPoolSize = GENERATE( 0, 1, 2 );

            auto& config = Configuration::getSynced();

            config.setSearchThreadPoolSize( threadPoolSize );
            config.setUseParallelSearch( threadPoolSize > 0 );

            auto filtered_lines = filtered_data->getNbLine();
            REQUIRE( filtered_lines.get() == 0 );

            SafeQSignalSpy searchProgressSpy{ filtered_data.get(),
                                              &LogFilteredData::searchProgressed };

            runSearch( filtered_data.get(), "this is line [0-9]{5}9", searchProgressSpy );

            THEN( "Matched lines are in data" )
            {
                QList<QVariant> progressArgs = searchProgressSpy.last();
                REQUIRE( qvariant_cast<LinesCount>( progressArgs.at( 0 ) ) == 50_lcount );

                const auto matches_count = filtered_data->getNbMatches();
                REQUIRE( matches_count == 50_lcount );

                const auto lines = filtered_data->getExpandedLines( 0_lnum, matches_count );
                for ( const auto& l : lines ) {
                    REQUIRE( l.endsWith( '9' ) );
                }
            }
        }
    }
}

SCENARIO( "marks and matches in filtered log data", "[logdata]" )
{
    LogDataLoader logDataLoader;

    GIVEN( "loaded log data" )
    {
        auto filtered_data = logDataLoader.log_data.getNewFilteredData();

        WHEN( "Searched for regex" )
        {
            auto& config = Configuration::getSynced();
            config.setSearchThreadPoolSize( 2 );
            config.setUseParallelSearch( true );

            auto filtered_lines = filtered_data->getNbLine();
            REQUIRE( filtered_lines.get() == 0 );

            SafeQSignalSpy searchProgressSpy{ filtered_data.get(),
                                              &LogFilteredData::searchProgressed };

            runSearch( filtered_data.get(), "this is line [0-9]{5}9", searchProgressSpy );
            
            AND_WHEN( "Add marks at matched line" )
            {
                const auto& firstMatchedLine = filtered_data->getLineString( 0_lnum );
                REQUIRE( firstMatchedLine.right( 2 ).toStdString() == "09" );

                filtered_data->addMark( 9_lnum );

                THEN( "Has same number of lines" )
                {
                    REQUIRE( filtered_data->getNbLine() == 50_lcount );
                }
            }

            AND_WHEN( "Add marks at not matched line" )
            {
                filtered_data->addMark( 5_lnum );

                THEN( "Has one more line" )
                {
                    REQUIRE( filtered_data->getNbLine() == 51_lcount );
                }
            }

            AND_WHEN( "Has mixed marks and matches" )
                {
                    filtered_data->addMark( 9_lnum );
                    filtered_data->addMark( 5_lnum );

                    AND_WHEN( "Only marks are visible" )
                    {
                        filtered_data->setVisibility( VisibilityFlags::Marks );

                        THEN( "Has only marked lines count" )
                        {
                            REQUIRE( filtered_data->getNbLine() == 2_lcount );
                        }

                        AND_WHEN( "Ask for line type by line" )
                        {
                            THEN( "Return mark" )
                            {
                                auto type = filtered_data->lineTypeByLine( 5_lnum );
                                REQUIRE( toFlags( type ) == LineTypeFlags::Mark );
                            }
                            THEN( "Return match" )
                            {
                                auto type = filtered_data->lineTypeByLine( 19_lnum );
                                REQUIRE( toFlags( type ) == LineTypeFlags::Match );
                            }

                            THEN( "Return mark & match" )
                            {
                                auto type = filtered_data->lineTypeByLine( 9_lnum );
                                REQUIRE( toFlags( type )
                                         == toFlags( LineTypeFlags::Mark | LineTypeFlags::Match ) );
                            }
                        }

                        WHEN( "Ask for line type by index" )
                        {
                            THEN( "Return mark" )
                            {
                                auto type = filtered_data->lineTypeByIndex( 0_lnum );
                                REQUIRE( toFlags( type ) == LineTypeFlags::Mark );
                            }
                            THEN( "Return mark & match" )
                            {
                                auto type = filtered_data->lineTypeByIndex( 1_lnum );
                                REQUIRE( toFlags( type )
                                         == toFlags( LineTypeFlags::Mark | LineTypeFlags::Match ) );
                            }
                        }
                    }

                    AND_WHEN( "Only matches are visible" )
                    {
                        filtered_data->setVisibility( VisibilityFlags::Matches );

                        THEN( "Has only matches lines count" )
                        {
                            REQUIRE( filtered_data->getNbLine() == 50_lcount );
                        }

                        AND_WHEN( "Ask for line type by line" )
                        {
                            THEN( "Return mark" )
                            {
                                auto type = filtered_data->lineTypeByLine( 5_lnum );
                                REQUIRE( toFlags( type ) == LineTypeFlags::Mark );
                            }
                            THEN( "Return match" )
                            {
                                auto type = filtered_data->lineTypeByLine( 19_lnum );
                                REQUIRE( toFlags( type ) == LineTypeFlags::Match );
                            }
                            THEN( "Return mark & match" )
                            {
                                auto type = filtered_data->lineTypeByLine( 9_lnum );
                                REQUIRE( toFlags( type )
                                         == toFlags( LineTypeFlags::Mark | LineTypeFlags::Match ) );
                            }
                        }

                        AND_WHEN( "Ask for line type by index" )
                        {
                            THEN( "Return match" )
                            {
                                auto type = filtered_data->lineTypeByIndex( 1_lnum );
                                REQUIRE( toFlags( type ) == LineTypeFlags::Match );
                            }
                            THEN( "Return mark & match" )
                            {
                                auto type = filtered_data->lineTypeByIndex( 0_lnum );
                                REQUIRE( toFlags( type )
                                         == toFlags( LineTypeFlags::Mark | LineTypeFlags::Match ) );
                            }
                        }
                    }

                    filtered_data->setVisibility( VisibilityFlags::Matches
                                                  | VisibilityFlags::Marks );

                    AND_WHEN( "Ask for line type by line" )
                    {
                        THEN( "Return Mark" )
                        {
                            auto type = filtered_data->lineTypeByLine( 5_lnum );
                            REQUIRE( toFlags( type ) == LineTypeFlags::Mark );
                        }
                        THEN( "Return match" )
                        {
                            auto type = filtered_data->lineTypeByLine( 19_lnum );
                            REQUIRE( toFlags( type ) == LineTypeFlags::Match );
                        }
                        THEN( "Return mark & match" )
                        {
                            auto type = filtered_data->lineTypeByLine( 9_lnum );
                            REQUIRE( toFlags( type )
                                     == toFlags( LineTypeFlags::Mark | LineTypeFlags::Match ) );
                        }
                    }

                    AND_WHEN( "Ask for line type by index" )
                    {
                        THEN( "Return mark" )
                        {
                            auto type = filtered_data->lineTypeByIndex( 0_lnum );
                            REQUIRE( toFlags( type ) == LineTypeFlags::Mark );
                        }
                        THEN( "Return match" )
                        {
                            auto type = filtered_data->lineTypeByIndex( 2_lnum );
                            REQUIRE( toFlags( type ) == LineTypeFlags::Match );
                        }
                        THEN( "Return mark & match" )
                        {
                            auto type = filtered_data->lineTypeByIndex( 1_lnum );
                            REQUIRE( toFlags( type )
                                     == toFlags( LineTypeFlags::Mark | LineTypeFlags::Match ) );
                        }
                    }
                }

            AND_WHEN( "Ask for matching line number" )
            {
                filtered_data->addMark( 1_lnum );

                AND_WHEN( "For marked line" )
                {
                    auto original_line = filtered_data->getMatchingLineNumber( 0_lnum );
                    THEN( "Original line is on mark" )
                    {
                        REQUIRE( original_line == 1_lnum );
                    }
                }

                AND_WHEN( "For matched line" )
                {
                    auto original_line = filtered_data->getMatchingLineNumber( 1_lnum );

                    const auto& firstMatchedLine = filtered_data->getLineString( 1_lnum );
                    REQUIRE( firstMatchedLine.right( 2 ).toStdString() == "09" );

                    THEN( "Original line is on match" )
                    {
                        REQUIRE( original_line == 9_lnum );
                    }
                }

                AND_WHEN( "For last line" )
                {
                    auto max_filtered_line = LineNumber( filtered_data->getNbLine().get() - 1 );
                    auto original_line = filtered_data->getMatchingLineNumber( max_filtered_line );
                    THEN( "Original line is last" )
                    {
                        REQUIRE( original_line == 499_lnum );
                    }
                }

                AND_WHEN( "For invalid line" )
                {
                    auto max_filtered_line = LineNumber( filtered_data->getNbLine().get() - 1 );
                    auto original_line
                        = filtered_data->getMatchingLineNumber( max_filtered_line + 1_lcount );
                    THEN( "Max line number is returned" )
                    {
                        REQUIRE( original_line == maxValue<LineNumber>() );
                    }
                }
            }

            AND_WHEN( "Ask for filtered line index" )
            {
                filtered_data->addMark( 1_lnum );

                AND_WHEN( "For marked line" )
                {
                    auto filtered_line = filtered_data->getLineIndexNumber( 1_lnum );
                    THEN( "Marked line returned" )
                    {
                        REQUIRE( filtered_line == 0_lnum );
                    }
                }

                AND_WHEN( "For matched line" )
                {
                    auto filtered_line = filtered_data->getLineIndexNumber( 9_lnum );
                    THEN( "Matched line returned" )
                    {
                        REQUIRE( filtered_line == 1_lnum );
                    }
                }

                AND_WHEN( "For last line" )
                {
                    auto max_filtered_line = LineNumber( filtered_data->getNbLine().get() - 1 );
                    auto filtered_line = filtered_data->getLineIndexNumber( 499_lnum );
                    THEN( "Last matched line returned" )
                    {
                        REQUIRE( filtered_line == max_filtered_line );
                    }
                }

                AND_WHEN( "For invalid line" )
                {
                    auto filtered_line = filtered_data->getMatchingLineNumber( 500_lnum );
                    THEN( "Max line number is returned" )
                    {
                        REQUIRE( filtered_line == maxValue<LineNumber>() );
                    }
                }
            }

            AND_WHEN( "Asked for line length" )
            {
                THEN( "Return expanded length" )
                {
                    REQUIRE( filtered_data->getLineLength( 1_lnum ) == LineLength( 92 ) );
                }
            }

            AND_WHEN( "Asked for line" )
            {
                THEN( "Return original line" )
                {
                    REQUIRE( filtered_data->getLineString( 2_lnum ).size() == 85 );
                }
            }

            AND_WHEN( "Asked for expanded line" )
            {
                THEN( "Return expanded line" )
                {
                    REQUIRE( filtered_data->getExpandedLineString( 2_lnum ).size() == 92 );
                }
            }

            AND_WHEN( "Asked to clear search" )
            {
                filtered_data->clearSearch();
                THEN( "Clear search results" )
                {
                    REQUIRE( filtered_data->getNbLine() == 0_lcount );
                }
            }
        }
    }
}
