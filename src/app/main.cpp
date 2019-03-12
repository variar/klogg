/*
 * Copyright (C) 2009, 2010, 2011, 2013, 2014 Nicolas Bonnefon and other contributors
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

#ifdef KLOGG_USE_TBB_MALLOC
#include <tbb/tbbmalloc_proxy.h>
#endif

#include <QApplication>
#include <QFileInfo>

#include <memory>

#include <iomanip>
#include <iostream>

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // _WIN32

#include "configuration.h"
#include "loadingstatus.h"
#include "highlighterset.h"
#include "mainwindow.h"
#include "persistentinfo.h"
#include "recentfiles.h"
#include "savedsearches.h"
#include "session.h"
#include "sessioninfo.h"
#include "versionchecker.h"

#include "log.h"

#include "messagereceiver.h"
#include "version.h"

#include <QtCore/QJsonDocument>
#include <QtWidgets/QApplication>

#include <cli11/cli11.hpp>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Appenders/RollingFileAppender.h>
#include <singleapp/singleapplication.h>

static void print_version();

int main( int argc, char* argv[] )
{
    SingleApplication app( argc, argv, true, SingleApplication::SecondaryNotification );

    // Register types for Qt
    qRegisterMetaType<LoadingStatus>( "LoadingStatus" );
    qRegisterMetaType<LinesCount>( "LinesCount" );
    qRegisterMetaType<LineNumber>( "LineNumber" );
    qRegisterMetaType<LineLength>( "LineLength" );

    std::vector<QString> filenames;

    // Configuration
    bool new_session = false;
    bool load_session = false;
    bool multi_instance = false;
    bool log_to_file = false;

    auto logLevel = logWARNING;
    CLI::App options{ "Klogg -- fast log explorer" };
    try {
        options.add_flag_function( "-v,--version",
                                   []( size_t ) {
                                       print_version();
                                       exit( 0 );
                                   },
                                   "print version information" );

        options.add_flag(
            "-m,--multi", multi_instance,
            "allow multiple instance of klogg to run simultaneously (use together with -s)" );
        options.add_flag( "-s,--load-session", load_session,
                          "load the previous session (default when no file is passed)" );
        options.add_flag( "-n,--new-session", new_session,
                          "do not load the previous session (default when a file is passed)" );

        options.add_flag( "-l,--log", log_to_file, "save the log to a file" );

        options.add_flag_function(
            "-d,--debug",
            [&logLevel]( size_t count ) {
                logLevel
                    = static_cast<std::decay<decltype( logLevel )>::type>( logWARNING + count );
            },
            "output more debug (include multiple times for more verbosity e.g. -dddd)" );

        options.add_option( "files",
            [&filenames](CLI::results_t res)
            {
                filenames.clear();
                for(const auto &a : res) {
                    filenames.emplace_back( QFile::decodeName( a.c_str() ) );
                }
                return !filenames.empty();
            }, "files to open" );

        options.parse( argc, argv );

    } catch ( const CLI::ParseError& e ) {
        return options.exit( e );
    } catch ( const std::exception& e ) {
        std::cerr << "Exception of unknown type: " << e.what() << std::endl;
    }

    std::unique_ptr<plog::IAppender> logAppender;

    if ( log_to_file ) {
        char file_name[ 255 ];
        snprintf( file_name, sizeof file_name, "klogg_%lld.log", app.applicationPid() );
        logAppender
            = std::make_unique<plog::RollingFileAppender<plog::GloggFormatter>>( file_name );
    }
    else {
        logAppender = std::make_unique<plog::ColorConsoleAppender<plog::GloggFormatter>>();
    }

    plog::init( logLevel, logAppender.get() );

    for ( auto& filename : filenames ) {
        if ( !filename.isEmpty() ) {
            // Convert to absolute path
            const QFileInfo file( filename );
            filename = file.absoluteFilePath();
            LOG( logDEBUG ) << "Filename: " << filename.toStdString();
        }
    }

    LOG( logINFO ) << "Klogg instance " << app.instanceId();

    MessageReceiver messageReceiver;

    if ( app.isSecondary() ) {
        LOG( logINFO ) << "Found another klogg, pid " << app.primaryPid();

        if ( !multi_instance ) {
#ifdef Q_OS_WIN
            ::AllowSetForegroundWindow( app.primaryPid() );
#endif
            QTimer::singleShot( 100, [&filenames, &app] {
                QStringList filesToOpen;

                for ( const auto& filename : filenames ) {
                    filesToOpen.append( filename );
                }

                QVariantMap data;
                data.insert( "version", GLOGG_VERSION );
                data.insert( "files", QVariant{ filesToOpen } );

                auto json = QJsonDocument::fromVariant( data );
                app.sendMessage( json.toBinaryData(), 5000 );

                QTimer::singleShot( 100, &app, &SingleApplication::quit );
            } );

            return app.exec();
        }
    }
    else {
        QObject::connect( &app, &SingleApplication::receivedMessage, &messageReceiver,
                          &MessageReceiver::receiveMessage, Qt::QueuedConnection );
    }

    // Register the configuration items
#ifdef KLOGG_PORTABLE
    GetPersistentInfo().migrateAndInit( PersistentInfo::Portable );
#else
    GetPersistentInfo().migrateAndInit( PersistentInfo::Common );
#endif

    GetPersistentInfo().registerPersistable( std::make_unique<SessionInfo>(),
                                             "session" );
    GetPersistentInfo().registerPersistable( std::make_unique<Configuration>(),
                                             "settings" );
    GetPersistentInfo().registerPersistable( std::make_unique<HighlighterSet>(),
                                             "HighlighterSet" );
    GetPersistentInfo().registerPersistable( std::make_unique<SavedSearches>(),
                                             "savedSearches" );
    GetPersistentInfo().registerPersistable( std::make_unique<RecentFiles>(),
                                             "recentFiles" );
    GetPersistentInfo().registerPersistable( std::make_unique<VersionCheckerConfig>(),
                                             "versionChecker" );

    // We support high-dpi (aka Retina) displays
    app.setAttribute( Qt::AA_UseHighDpiPixmaps );

    // No icon in menus
    app.setAttribute( Qt::AA_DontShowIconsInMenus );

    // FIXME: should be replaced by a two staged init of MainWindow
    GetPersistentInfo().retrieve( "settings" );

    std::unique_ptr<Session> session( new Session() );
    MainWindow mw( std::move( session ) );

    if ( app.isPrimary() ) {
        QObject::connect( &messageReceiver, &MessageReceiver::loadFile, &mw,
                          &MainWindow::loadFileNonInteractive );
    }

    // Geometry
    mw.reloadGeometry();

    // Load the existing session if needed
    const auto& config = Persistent<Configuration>( "settings" );
    if ( load_session || ( filenames.empty() && !new_session && config.loadLastSession() ) )
        mw.reloadSession();

    LOG( logDEBUG ) << "MainWindow created.";
    mw.show();

    for ( const auto& filename : filenames ) {
        mw.loadInitialFile( filename );
    }

    mw.startBackgroundTasks();

    return app.exec();
}

static void print_version()
{
    std::cout << "glogg " GLOGG_VERSION "\n";
#ifdef GLOGG_COMMIT
    std::cout << "Built " GLOGG_DATE " from " GLOGG_COMMIT "(" GLOGG_GIT_VERSION ")\n";
#endif
    std::cout << "Copyright (C) 2019 Nicolas Bonnefon, Anton Filimonov and other contributors\n";
    std::cout << "This is free software.  You may redistribute copies of it under the terms of\n";
    std::cout << "the GNU General Public License <http://www.gnu.org/licenses/gpl.html>.\n";
    std::cout << "There is NO WARRANTY, to the extent permitted by law.\n";
}
