#include <QTest>
#include <QSignalSpy>

#include "log.h"
#include "test_utils.h"

#include "logdata.h"

#include "gmock/gmock.h"

#define TMPDIR "/tmp"

static const qint64 VBL_NB_LINES = 4999999LL;
static const int VBL_LINE_PER_PAGE = 70;
static const char* vbl_format="LOGDATA is a part of glogg, we are going to test it thoroughly, this is line\t\t%07d\n";
static const int VBL_LINE_LENGTH = (76+2+7) ; // Without the final '\n' !
static const int VBL_VISIBLE_LINE_LENGTH = (76+8+4+7); // Without the final '\n' !

class PerfLogData : public testing::Test {
  public:
    PerfLogData() {
        plog::get()->setMaxSeverity( logERROR );
        generateDataFiles();
    }

    bool generateDataFiles() {
        char newLine[90];

        QFile file( TMPDIR "/verybiglog.txt" );
        if ( file.open( QIODevice::WriteOnly ) ) {
            for (int i = 0; i < VBL_NB_LINES; i++) {
                snprintf(newLine, 89, vbl_format, i);
                file.write( newLine, qstrlen(newLine) );
            }
        }
        else {
            return false;
        }
        file.close();

        return true;
    }
};

TEST_F( PerfLogData, simpleLoad ) {
    LogData log_data;
    QSignalSpy progressSpy( &log_data, SIGNAL( loadingProgressed( int ) ) );
    QSignalSpy endSpy( &log_data, SIGNAL( loadingFinished( LoadingStatus ) ) );

    {
        TestTimer t;

        log_data.attachFile( TMPDIR "/verybiglog.txt" );
        ASSERT_TRUE( endSpy.wait( 20000 ) );
    }

    ASSERT_THAT( log_data.getNbLine(), VBL_NB_LINES );
    ASSERT_THAT( log_data.getMaxLength(), VBL_VISIBLE_LINE_LENGTH );
    ASSERT_THAT( log_data.getLineLength( 123 ), VBL_VISIBLE_LINE_LENGTH );
    ASSERT_THAT( log_data.getFileSize(), VBL_NB_LINES * (VBL_LINE_LENGTH+1LL) );

    // Blocks of 5 MiB + 1 for the start notification (0%)
    ASSERT_THAT( progressSpy.count(), log_data.getFileSize() / (5LL*1024*1024) + 2 );
    ASSERT_THAT( endSpy.count(), 1 );
    QList<QVariant> arguments = endSpy.takeFirst();
    ASSERT_THAT( arguments.at(0).toInt(),
            static_cast<int>( LoadingStatus::Successful ) );
}

class PerfLogDataRead : public PerfLogData {
  public:
    PerfLogDataRead() : PerfLogData(), log_data(), endSpy(
            &log_data, SIGNAL( loadingFinished( LoadingStatus ) ) ) {
        log_data.attachFile( TMPDIR "/verybiglog.txt" );
        endSpy.wait( 20000 );
    }

    LogData log_data;
    SafeQSignalSpy endSpy;
};

TEST_F( PerfLogDataRead, sequentialRead ) {
    ASSERT_THAT( log_data.getNbLine(), VBL_NB_LINES );
    // Read all lines sequentially
    QString s;
    {
        TestTimer t;

        for (int i = 0; i < VBL_NB_LINES; i++) {
            s = log_data.getLineString(i);
        }
    }
}

TEST_F( PerfLogDataRead, sequentialReadExpanded ) {
    ASSERT_THAT( log_data.getNbLine(), VBL_NB_LINES );
    // Read all lines sequentially (expanded)
    QString s;
    {
        TestTimer t;

        for (int i = 0; i < VBL_NB_LINES; i++) {
            s = log_data.getExpandedLineString(i);
        }
    }
}

TEST_F( PerfLogDataRead, randomPageRead ) {
    ASSERT_THAT( log_data.getNbLine(), VBL_NB_LINES );
    // Read page by page from the beginning and the end, using the QStringList
    // function
    QStringList list;
    {
        TestTimer t;

        for (int page = 0; page < (VBL_NB_LINES/VBL_LINE_PER_PAGE)-1; page++)
        {
            list = log_data.getLines( page*VBL_LINE_PER_PAGE, VBL_LINE_PER_PAGE );
            ASSERT_THAT(list.count(), VBL_LINE_PER_PAGE);
            int page_from_end = (VBL_NB_LINES/VBL_LINE_PER_PAGE) - page - 1;
            list = log_data.getLines( page_from_end*VBL_LINE_PER_PAGE, VBL_LINE_PER_PAGE );
            ASSERT_THAT(list.count(), VBL_LINE_PER_PAGE);
        }
    }
}

TEST_F( PerfLogDataRead, randomPageReadExpanded ) {
    ASSERT_THAT( log_data.getNbLine(), VBL_NB_LINES );
    // Read page by page from the beginning and the end, using the QStringList
    // function
    QStringList list;
    {
        TestTimer t;

        for (int page = 0; page < (VBL_NB_LINES/VBL_LINE_PER_PAGE)-1; page++)
        {
            list = log_data.getExpandedLines( page*VBL_LINE_PER_PAGE, VBL_LINE_PER_PAGE );
            ASSERT_THAT(list.count(), VBL_LINE_PER_PAGE);
            int page_from_end = (VBL_NB_LINES/VBL_LINE_PER_PAGE) - page - 1;
            list = log_data.getExpandedLines( page_from_end*VBL_LINE_PER_PAGE, VBL_LINE_PER_PAGE );
            ASSERT_THAT(list.count(), VBL_LINE_PER_PAGE);
        }
    }
}
