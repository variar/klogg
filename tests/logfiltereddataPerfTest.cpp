#include <QTest>
#include <QSignalSpy>

#include "log.h"
#include "test_utils.h"

#include "logdata.h"
#include "logfiltereddata.h"

#include "gmock/gmock.h"

#define TMPDIR "/tmp"

static const qint64 VBL_NB_LINES = 4999999LL;
static const int VBL_LINE_PER_PAGE = 70;
static const char* vbl_format="LOGDATA is a part of glogg, we are going to test it thoroughly, this is line\t\t%07d\n";
static const int VBL_LINE_LENGTH = (76+2+7) ; // Without the final '\n' !
static const int VBL_VISIBLE_LINE_LENGTH = (76+8+4+7); // Without the final '\n' !

class PerfLogFilteredData : public testing::Test {
  public:
    PerfLogFilteredData()
        : log_data_(), filtered_data_( log_data_.getNewFilteredData() ),
          progressSpy( filtered_data_.get(), SIGNAL( searchProgressed( int, int ) ) ) {

        plog::get()->setMaxSeverity( logERROR );

        generateDataFiles();

        QSignalSpy endSpy( &log_data_, SIGNAL( loadingFinished( LoadingStatus ) ) );

        log_data_.attachFile( TMPDIR "/verybiglog.txt" );
        if ( ! endSpy.wait( 999000 ) )
            std::cerr << "Unable to attach the file!";
    }

    LogData log_data_;
    std::unique_ptr<LogFilteredData> filtered_data_;
    QSignalSpy progressSpy;

    void search() {
        int percent = 0;
        do {
            if ( progressSpy.wait( 10000 ) )
                percent = qvariant_cast<int>(progressSpy.last().at(1));
            else
                std::cout << "Missed...\n";
            // std::cout << "Progress " << percent << std::endl;
        } while ( percent < 100 );
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

TEST_F( PerfLogFilteredData, allMatchingSearch ) {
    {
        TestTimer t;
        filtered_data_->runSearch( QRegularExpression( "glogg.*this" ) );
        search();
    }
    ASSERT_THAT( filtered_data_->getNbLine(), VBL_NB_LINES );
}

TEST_F( PerfLogFilteredData, someMatchingSearch ) {
    {
        TestTimer t;
        filtered_data_->runSearch( QRegularExpression( "1?3|34" ) );
        search();
    }
    ASSERT_THAT( filtered_data_->getNbLine(), 2874236 );
}

TEST_F( PerfLogFilteredData, noneMatchingSearch ) {
    {
        TestTimer t;
        filtered_data_->runSearch( QRegularExpression( "a1?3|(w|f)f34|blah" ) );
        search();
    }
    ASSERT_THAT( filtered_data_->getNbLine(), 0 );
}

TEST_F( PerfLogFilteredData, browsingSearchResults ) {
    filtered_data_->runSearch( QRegularExpression( "1?3|34" ) );
    search();
    ASSERT_THAT( filtered_data_->getNbLine(), 2874236 );

    // Read page by page from the beginning and the end, using the QStringList
    // function
    std::cout << "Start reading..." << std::endl;
    QStringList list;
    {
        TestTimer t;

        const int nb_results = filtered_data_->getNbLine();
        for (int page = 0; page < (nb_results/VBL_LINE_PER_PAGE)-1; page++)
        {
            list = filtered_data_->getExpandedLines(
                    page * VBL_LINE_PER_PAGE, VBL_LINE_PER_PAGE );
            ASSERT_THAT(list.count(), VBL_LINE_PER_PAGE);
            int page_from_end = (nb_results/VBL_LINE_PER_PAGE) - page - 1;
            list = filtered_data_->getExpandedLines(
                    page_from_end * VBL_LINE_PER_PAGE, VBL_LINE_PER_PAGE );
            ASSERT_THAT(list.count(), VBL_LINE_PER_PAGE);
        }
    }
}

