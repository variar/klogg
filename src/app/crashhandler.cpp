/*
 * Copyright (C) 2020, 2021 Anton Filimonov and other contributors
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

#include "crashhandler.h"

#include <QByteArray>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProgressDialog>
#include <QPushButton>
#include <QStandardPaths>
#include <QSysInfo>
#include <QTimer>
#include <QUrlQuery>
#include <QVBoxLayout>
#include <cstdint>
#include <cstdlib>
#include <string_view>

#ifdef KLOGG_USE_MIMALLOC
#include <mimalloc.h>
#endif

#include "client/crash_report_database.h"
#include "sentry.h"

#include "cpu_info.h"
#include "issuereporter.h"
#include "klogg_version.h"
#include "log.h"
#include "memory_info.h"
#include "openfilehelper.h"

namespace {

constexpr const char* DSN
    = "https://aad3b270e5ba4ec2915eb5caf6e6d929@o453796.ingest.sentry.io/5442855";

QString sentryDatabasePath()
{
#ifdef KLOGG_PORTABLE
    auto basePath = QCoreApplication::applicationDirPath();
#else
    auto basePath = QStandardPaths::writableLocation( QStandardPaths::AppDataLocation );
#endif

    return basePath.append( "/klogg_dump" );
}

void logSentry( sentry_level_t level, const char* message, va_list args, void* userdata )
{
    Q_UNUSED( userdata );
    plog::Severity severity;
    switch ( level ) {
    case SENTRY_LEVEL_WARNING:
        severity = plog::Severity::warning;
        break;
    case SENTRY_LEVEL_ERROR:
        severity = plog::Severity::error;
        break;
    default:
        severity = plog::Severity::info;
        break;
    }

    PLOG( severity ).printf( message, args );
}

QDialog::DialogCode askUserConfirmation( const QString& formattedReport, const QString& reportPath )
{
    auto message = std::make_unique<QLabel>();
    message->setText( "During last run application has encountered an unexpected error." );

    auto crashReportHeader = std::make_unique<QLabel>();
    crashReportHeader->setText( "We collected the following crash report:" );

    auto report = std::make_unique<QPlainTextEdit>();
    report->setReadOnly( true );
    report->setPlainText( formattedReport );
    report->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

    auto sendReportLabel = std::make_unique<QLabel>();
    sendReportLabel->setText( "Application can send this report to sentry.io for developers to "
                              "analyze and fix the issue" );

    auto privacyPolicy = std::make_unique<QLabel>();
    privacyPolicy->setText(
        "<a href=\"https://klogg.filimonov.dev/docs/privacy_policy\">Privacy policy</a>" );

    privacyPolicy->setTextFormat( Qt::RichText );
    privacyPolicy->setTextInteractionFlags( Qt::TextBrowserInteraction );
    privacyPolicy->setOpenExternalLinks( true );

    auto exploreButton = std::make_unique<QPushButton>();
    exploreButton->setText( "Open report directory" );
    exploreButton->setFlat( true );
    QObject::connect( exploreButton.get(), &QPushButton::clicked,
                      [ &reportPath ] { showPathInFileExplorer( reportPath ); } );

    auto privacyLayout = std::make_unique<QHBoxLayout>();
    privacyLayout->addWidget( privacyPolicy.release() );
    privacyLayout->addStretch();
    privacyLayout->addWidget( exploreButton.release() );

    auto buttonBox = std::make_unique<QDialogButtonBox>();
    buttonBox->addButton( "Send report", QDialogButtonBox::AcceptRole );
    buttonBox->addButton( "Discard report", QDialogButtonBox::RejectRole );

    auto confirmationDialog = std::make_unique<QDialog>();
    confirmationDialog->resize( 800, 600 );

    QObject::connect( buttonBox.get(), &QDialogButtonBox::accepted, confirmationDialog.get(),
                      &QDialog::accept );
    QObject::connect( buttonBox.get(), &QDialogButtonBox::rejected, confirmationDialog.get(),
                      &QDialog::reject );

    auto layout = std::make_unique<QVBoxLayout>();
    layout->addWidget( message.release() );
    layout->addWidget( crashReportHeader.release() );
    layout->addWidget( report.release() );
    layout->addWidget( sendReportLabel.release() );
    layout->addLayout( privacyLayout.release() );
    layout->addWidget( buttonBox.release() );

    confirmationDialog->setLayout( layout.release() );

    return static_cast<QDialog::DialogCode>( confirmationDialog->exec() );
}

bool checkCrashpadReports( const QString& databasePath )
{
    using namespace crashpad;

    bool needWaitForUpload = false;

#ifdef Q_OS_WIN
    auto database = CrashReportDatabase::InitializeWithoutCreating(
        base::FilePath{ databasePath.toStdWString() } );
#else
    auto database = CrashReportDatabase::InitializeWithoutCreating(
        base::FilePath{ databasePath.toStdString() } );
#endif

    std::vector<CrashReportDatabase::Report> pendingReports;
    database->GetCompletedReports( &pendingReports );
    LOG_INFO << "Pending reports " << pendingReports.size();

#ifdef Q_OS_WIN
    const auto stackwalker = QCoreApplication::applicationDirPath() + "/klogg_minidump_dump.exe";
#else
    const auto stackwalker = QCoreApplication::applicationDirPath() + "/klogg_minidump_dump";
#endif

    for ( const auto& report : pendingReports ) {
        if ( report.uploaded ) {
            continue;
        }

#ifdef Q_OS_WIN
        const auto reportFile = QString::fromStdWString( report.file_path.value() );
#else
        const auto reportFile = QString::fromStdString( report.file_path.value() );
#endif

        QProcess stackProcess;
        stackProcess.start( stackwalker, QStringList() << reportFile );
        stackProcess.waitForFinished();

        QString formattedReport = reportFile;
        formattedReport.append( QChar::LineFeed )
            .append( QString::fromUtf8( stackProcess.readAllStandardOutput() ) );

        if ( QDialog::Accepted == askUserConfirmation( formattedReport, reportFile ) ) {
            database->RequestUpload( report.uuid );
            needWaitForUpload = true;
        }
        else {
            database->DeleteReport( report.uuid );
        }

        IssueReporter::askUserAndReportIssue( IssueTemplate::Crash,
                                              report.uuid.ToString().c_str() );
    }
    return needWaitForUpload;
}
} // namespace

CrashHandler::CrashHandler()
{
    const auto dumpPath = sentryDatabasePath();
    const auto hasDumpDir = QDir{ dumpPath }.mkpath( "." );

    const auto needWaitForUpload = hasDumpDir ? checkCrashpadReports( dumpPath ) : false;

    sentry_options_t* sentryOptions = sentry_options_new();

    sentry_options_set_logger( sentryOptions, logSentry, nullptr );
    sentry_options_set_debug( sentryOptions, 1 );

#ifdef Q_OS_WIN
    const auto handlerPath = QCoreApplication::applicationDirPath() + "/klogg_crashpad_handler.exe";
    sentry_options_set_database_pathw( sentryOptions, dumpPath.toStdWString().c_str() );
    sentry_options_set_handler_pathw( sentryOptions, handlerPath.toStdWString().c_str() );
#else
    const auto handlerPath = QCoreApplication::applicationDirPath() + "/klogg_crashpad_handler";
    sentry_options_set_database_path( sentryOptions, dumpPath.toStdString().c_str() );
    sentry_options_set_handler_path( sentryOptions, handlerPath.toStdString().c_str() );
#endif

    sentry_options_set_dsn( sentryOptions, DSN );

    // klogg asks confirmation and sends reports using crashpad
    sentry_options_set_require_user_consent( sentryOptions, true );

    sentry_options_set_auto_session_tracking( sentryOptions, false );

    sentry_options_set_symbolize_stacktraces( sentryOptions, true );

    sentry_options_set_environment( sentryOptions, "development" );
    sentry_options_set_release( sentryOptions, kloggVersion().data() );

    sentry_init( sentryOptions );

    sentry_set_tag( "commit", kloggCommit().data() );
    sentry_set_tag( "qt", qVersion() );
    sentry_set_tag( "build_arch", QSysInfo::buildCpuArchitecture().toLatin1().data() );

    auto addExtra = []( const char* name, auto value ) {
        sentry_set_extra( name, sentry_value_new_string( std::to_string( value ).c_str() ) );
        LOG_INFO << "Process stats: " << name << " - " << value;
    };

    addExtra( "memory", physicalMemory() );

    addExtra( "cpuInstructions", static_cast<unsigned>( supportedCpuInstructions() ) );

    memoryUsageTimer_ = std::make_unique<QTimer>();
    QObject::connect( memoryUsageTimer_.get(), &QTimer::timeout, [ addExtra ]() {
        const auto vmUsed = usedMemory();
        addExtra( "vm_used", vmUsed );

#ifdef KLOGG_USE_MIMALLOC
        size_t elapsedMsecs, userMsecs, systemMsecs, currentRss, peakRss, currentCommit, peakCommit,
            pageFaults;

        mi_process_info( &elapsedMsecs, &userMsecs, &systemMsecs, &currentRss, &peakRss,
                         &currentCommit, &peakCommit, &pageFaults );

        addExtra( "elapsed_msecs", elapsedMsecs );
        addExtra( "user_msecs", userMsecs );
        addExtra( "system_msecs", systemMsecs );
        addExtra( "current_rss", currentRss );
        addExtra( "peak_rss", peakRss );
        addExtra( "current_commit", currentCommit );
        addExtra( "peak_commit", peakCommit );
        addExtra( "page_faults", pageFaults );
#endif
    } );
    memoryUsageTimer_->start( 10000 );

    if ( needWaitForUpload ) {
        QProgressDialog progressDialog;
        progressDialog.setLabelText( "Uploading crash reports" );
        progressDialog.setRange( 0, 0 );

        QTimer::singleShot( 30 * 1000, &progressDialog, &QProgressDialog::cancel );
        progressDialog.exec();
    }
}

CrashHandler::~CrashHandler()
{
    memoryUsageTimer_->stop();
    sentry_shutdown();
}
