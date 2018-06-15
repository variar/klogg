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

#include <QTest>
#include <QSignalSpy>
#include <QTemporaryFile>

#include "configuration.h"
#include "log.h"
#include "persistentinfo.h"
#include "test_utils.h"

#include "logdata.h"
#include "logfiltereddata.h"

#include <catch.hpp>

static const qint64 SL_NB_LINES = 500LL;
static const char* sl_format="LOGDATA \t is a part of glogg, we are going to test it thoroughly, this is line %06d\n";

namespace {

bool generateDataFiles(QTemporaryFile& file) {
    char newLine[90];

    if ( file.open() ) {
        for (int i = 0; i < SL_NB_LINES; i++) {
            snprintf(newLine, 89, sl_format, i);
            file.write( newLine, qstrlen(newLine) );
        }
        file.flush();
    }

    return true;
}

void runSearch( LogFilteredData* filtered_data,  const QString& regexp,
                SafeQSignalSpy& searchProgressSpy ) {

    filtered_data->runSearch( QRegularExpression( regexp ) );

    int progress = 0;
    do
    {
        REQUIRE( searchProgressSpy.wait() );
        QList<QVariant> progressArgs = searchProgressSpy.last();
        progress = progressArgs.at(1).toInt();
    } while ( progress < 100 );
}

}

SCENARIO( "filtered log data", "[logdata]") {
    using Visibility = LogFilteredData::Visibility;
    using LineType = LogFilteredData::FilteredLineType;

    GIVEN( "loaded log data" ) {
        QTemporaryFile file;
        REQUIRE( generateDataFiles( file ) );
        LogData log_data;
        SafeQSignalSpy loadEndSpy( &log_data, SIGNAL( loadingFinished( LoadingStatus ) ) );

        log_data.attachFile( file.fileName() );
        REQUIRE( loadEndSpy.safeWait( 10000 ) );

        auto filtered_data = std::unique_ptr<LogFilteredData>( log_data.getNewFilteredData() );

        WHEN( "Adding mark outside file" ) {
            filtered_data->addMark( LineNumber( SL_NB_LINES + 25 ) );

            THEN( "No marked lines stored" ) {
                for ( LineNumber i = 0_lnum; i < LineNumber( SL_NB_LINES ); ++i )
                    REQUIRE_FALSE( filtered_data->isLineMarked( i ) );
            }
        }

        WHEN( "Adding marks in log file" ) {
            filtered_data->addMark( 10_lnum );
            filtered_data->addMark( 25_lnum );

            WHEN( "Check for marked line") {
                THEN( "Return true") {
                    REQUIRE( filtered_data->isLineMarked( 10_lnum ) );
                    REQUIRE( filtered_data->isLineMarked( 25_lnum ) );
                }
            }

            WHEN( "Get marks count" ) {
                THEN( "Return all marks count" ) {
                    REQUIRE( filtered_data->getNbMarks() == 2_lcount );
                }
            }

            WHEN( "Get marks" ) {
                auto marks = filtered_data->getMarks();
                THEN( "Provide all marks" ) {
                    REQUIRE( marks.size() == 2 );
                }
            }

            WHEN ( "Get mark before has mark" ) {
                const auto markBefore = filtered_data->getMarkBefore( 25_lnum );
                THEN( "Return previous mark" ) {
                    REQUIRE( markBefore.has_value() );
                    REQUIRE( *markBefore == 10_lnum );
                }
            }

            WHEN ( "Get mark before has no data" ) {
                const auto markBefore = filtered_data->getMarkBefore( 10_lnum );
                THEN( "Return no mark" ) {
                    REQUIRE_FALSE( markBefore.has_value() );
                }
            }

            WHEN ( "Get mark after has mark" ) {
                const auto markAfter = filtered_data->getMarkAfter( 10_lnum );
                THEN( "Return next mark" ) {
                    REQUIRE( markAfter.has_value() );
                    REQUIRE( *markAfter == 25_lnum );
                }
            }

            WHEN ( "Get mark after has no data" ) {
                const auto markAfter = filtered_data->getMarkAfter( 25_lnum );
                THEN( "Return no mark" ) {
                    REQUIRE_FALSE( markAfter.has_value() );
                }
            }

            WHEN( "Delete mark" ) {
                filtered_data->deleteMark( 10_lnum );
                THEN( "Mark is removed" ) {
                    REQUIRE_FALSE( filtered_data->isLineMarked( 10_lnum ) );
                    REQUIRE( filtered_data->getNbMarks() == 1_lcount );
                }
            }

            WHEN( "Clear marks" ) {
                filtered_data->clearMarks();
                THEN( "All marks are removed" ) {
                    REQUIRE( filtered_data->getNbMarks() == 0_lcount );
                }
            }
        }

        WHEN( "Searched for regex" ) {

            const auto threadPoolSize = GENERATE(0, 1, 2, 3);

            auto& config = Persistent<Configuration>( "settings" );
            config.setSearchThreadPoolSize( threadPoolSize );    
            config.setUseParallelSearch( threadPoolSize > 0 );    

            auto filtered_lines = filtered_data->getNbLine();
            REQUIRE( filtered_lines.get() == 0);

            SafeQSignalSpy searchProgressSpy {
                        filtered_data.get(),
                        &LogFilteredData::searchProgressed
            };

            runSearch( filtered_data.get(), "this is line [0-9]{5}9", searchProgressSpy );

            THEN( "Matched lines are in data" ) {
                QList<QVariant> progressArgs = searchProgressSpy.last();
                REQUIRE( qvariant_cast<LinesCount>( progressArgs.at(0) ) == 50_lcount );

                const auto matches_count = filtered_data->getNbMatches();
                REQUIRE( matches_count == 50_lcount );

                const auto lines = filtered_data->getExpandedLines(0_lnum,
                                                                   matches_count );
                for ( const auto& l: lines) {
                    REQUIRE( l.endsWith( '9' ) );
                }
            }

            WHEN( "Add marks at matched line" ) {
                const auto& firstMatchedLine = filtered_data->getLineString(0_lnum);
                REQUIRE( firstMatchedLine.right(2).toStdString() == "09");

                filtered_data->addMark( 9_lnum );

                THEN( "Has same number of lines" ) {
                    REQUIRE( filtered_data->getNbLine() == 50_lcount );
                }
            }

            WHEN( "Add marks at not matched line" ) {
                filtered_data->addMark( 5_lnum );

                THEN( "Has one more line" ) {
                    REQUIRE( filtered_data->getNbLine() == 51_lcount );
                }
            }

            WHEN( "Has mixed marks and mathes" ) {
                filtered_data->addMark( 9_lnum );
                filtered_data->addMark( 5_lnum );

                WHEN( "Only marks are visible" ) {

                    filtered_data->setVisibility( Visibility::MarksOnly );

                    THEN( "Has only marked lines count" ) {
                        REQUIRE( filtered_data->getNbLine() == 2_lcount );
                    }
                }

                WHEN( "Only mathes are visible" ) {
                    filtered_data->setVisibility( Visibility::MatchesOnly );

                    THEN( "Has only matches lines count" ) {
                        REQUIRE( filtered_data->getNbLine() == 50_lcount );
                    }
                }

                WHEN( "Ask for marked line type" ) {
                    auto type = filtered_data->filteredLineTypeByIndex( 0_lnum );
                    THEN( "Return mark" ) {
                        REQUIRE( type == LineType::Mark );
                    }
                }

                WHEN( "Ask for matched line type" ) {
                    auto type = filtered_data->filteredLineTypeByIndex( 50_lnum );
                    THEN( "Return match" ) {
                        REQUIRE( type == LineType::Match );
                    }
                }

                WHEN( "Ask for mixed line type" ) {
                    auto type = filtered_data->filteredLineTypeByIndex( 1_lnum );
                    THEN( "Return mark and match" ) {
                        REQUIRE( type == ( LineType::Mark | LineType::Match ) );
                    }
                }
            }

            WHEN( "Ask for matching line number" ) {
                filtered_data->addMark( 1_lnum );

                WHEN( "For marked line" ) {
                    auto original_line = filtered_data->getMatchingLineNumber( 0_lnum );
                    THEN( "Original line is on mark" ) {
                        REQUIRE( original_line == 1_lnum );
                    }
                }

                WHEN( "For matched line" ) {
                    auto original_line = filtered_data->getMatchingLineNumber( 1_lnum );

                    const auto& firstMatchedLine = filtered_data->getLineString(1_lnum);
                    REQUIRE( firstMatchedLine.right(2).toStdString() == "09");

                    THEN( "Original line is on match" ) {
                        REQUIRE( original_line == 9_lnum );
                    }
                }

                WHEN( "For last line" ) {
                    auto max_filtered_line = LineNumber( filtered_data->getNbLine().get() - 1 );
                    auto original_line = filtered_data->getMatchingLineNumber( max_filtered_line );
                    THEN( "Original line is last" ) {
                        REQUIRE( original_line == 499_lnum );
                    }
                }

                WHEN( "For invalid line" ) {
                    auto max_filtered_line = LineNumber( filtered_data->getNbLine().get() - 1 );
                    auto original_line = filtered_data->getMatchingLineNumber( max_filtered_line + 1_lcount );
                    THEN( "Max line number is returned" ) {
                        REQUIRE( original_line == maxValue<LineNumber>() );
                    }
                }
            }

            WHEN( "Ask for filtered line index" ) {
                filtered_data->addMark( 1_lnum );

                WHEN( "For marked line" ) {
                    auto filtered_line = filtered_data->getLineIndexNumber( 1_lnum );
                    THEN( "Marked line returned" ) {
                        REQUIRE( filtered_line == 0_lnum );
                    }
                }

                WHEN( "For matched line" ) {
                    auto filtered_line = filtered_data->getLineIndexNumber( 9_lnum );
                    THEN( "Matched line returned" ) {
                        REQUIRE( filtered_line == 1_lnum );
                    }
                }

                WHEN( "For last line" ) {
                    auto max_filtered_line = LineNumber( filtered_data->getNbLine().get() - 1 );
                    auto filtered_line = filtered_data->getLineIndexNumber( 499_lnum );
                    THEN( "Last matched line returned" ) {
                        REQUIRE( filtered_line == max_filtered_line );
                    }
                }

                WHEN( "For invalid line" ) {
                    auto filtered_line = filtered_data->getMatchingLineNumber( 500_lnum );
                    THEN( "Max line number is returned" ) {
                        REQUIRE( filtered_line == maxValue<LineNumber>() );
                    }
                }
            }

            WHEN( "Asked for line length" ) {
                THEN("Return expanded length") {
                    REQUIRE( filtered_data->getLineLength( 1_lnum ) == LineLength( 92 ) );
                }
            }

            WHEN( "Asked for line" ) {
                THEN("Return original line") {
                    REQUIRE( filtered_data->getLineString( 2_lnum ).size() == 85 );
                }
            }

            WHEN( "Asked for expanded line" ) {
                THEN( "Return expanded line" ) {
                    REQUIRE( filtered_data->getExpandedLineString( 2_lnum ).size() == 92 );
                }
            }

            WHEN( "Asked to clear search" ) {
                filtered_data->clearSearch();
                THEN( "Clear search results" ) {
                    REQUIRE( filtered_data->getNbLine() == 0_lcount );
                }
            }
        }
    }
}
