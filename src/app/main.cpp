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
 * Copyright (C) 2016 -- 2021 Anton Filimonov and other contributors
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

#include "log.h"
#include <QMessageBox>
#include <QtGlobal>
#include <qapplication.h>
#include <qthreadpool.h>

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#if defined( KLOGG_USE_MIMALLOC )
#include <mimalloc-new-delete.h>
#endif
#endif // _WIN32

#if defined( KLOGG_USE_TBBMALLOC )
#include <tbb/tbbmalloc_proxy.h>
#elif defined( KLOGG_USE_MIMALLOC )
#include <mimalloc.h>
#endif

#include "tbb/global_control.h"

#include "configuration.h"
#include "cpu_info.h"
#include "logger.h"
#include "mainwindow.h"
#include "styles.h"

#include "cli.h"
#include "kloggapp.h"

#ifdef KLOGG_PORTABLE
const bool PersistentInfo::ForcePortable = true;
#else
const bool PersistentInfo::ForcePortable = false;
#endif

void setApplicationAttributes( bool enableQtHdpi, int scaleFactorRounding )
{
    // When QNetworkAccessManager is instantiated it regularly starts polling
    // all network interfaces to see if anything changes and if so, what. This
    // creates a latency spike every 10 seconds on Mac OS 10.12+ and Windows 7 >=
    // when on a wifi connection.
    // So here we disable it for lack of better measure.
    // This will also cause this message: QObject::startTimer: Timers cannot
    // have negative intervals
    // For more info see:
    // - https://bugreports.qt.io/browse/QTBUG-40332
    // - https://bugreports.qt.io/browse/QTBUG-46015
    qputenv( "QT_BEARER_POLL_TIMEOUT", QByteArray::number( std::numeric_limits<int>::max() ) );

#if QT_VERSION < QT_VERSION_CHECK( 6, 0, 0 )
#ifdef Q_OS_WIN
    QCoreApplication::setAttribute( Qt::AA_DisableWindowContextHelpButton );
#endif

    if ( !enableQtHdpi ) {
        QCoreApplication::setAttribute( Qt::AA_DisableHighDpiScaling );
    }
    else {

#if QT_VERSION >= QT_VERSION_CHECK( 5, 14, 0 )
        QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
            static_cast<Qt::HighDpiScaleFactorRoundingPolicy>( scaleFactorRounding ) );
#else
        Q_UNUSED( scaleFactorRounding );
#endif

        // This attribute must be set before QGuiApplication is constructed:
        QCoreApplication::setAttribute( Qt::AA_EnableHighDpiScaling );
        // We support high-dpi (aka Retina) displays
        QCoreApplication::setAttribute( Qt::AA_UseHighDpiPixmaps );
    }
#else
    Q_UNUSED( enableQtHdpi );
    Q_UNUSED( scaleFactorRounding );
#endif

    QCoreApplication::setAttribute( Qt::AA_DontShowIconsInMenus );
}

int main( int argc, char* argv[] )
{
#ifdef KLOGG_USE_MIMALLOC
    mi_process_init();
#endif

    const auto& config = Configuration::getSynced();
    setApplicationAttributes( config.enableQtHighDpi(), config.scaleFactorRounding() );

    KloggApp app( argc, argv );
    MainWindow::installLanguage( config.language() );
    CliParameters parameters( app );

    const auto logLevel
        = static_cast<logging::LogLevel>( std::max( parameters.log_level, config.loggingLevel() ) );
    logging::enableLogging( parameters.enable_logging || config.enableLogging(), logLevel );
    logging::enableFileLogging( parameters.log_to_file || config.enableLogging(), logLevel );

    app.initCrashHandler();

    auto maxConcurrency
        = tbb::global_control::active_value( tbb::global_control::max_allowed_parallelism );

    LOG_INFO << "Klogg instance " << app.instanceId()
#ifdef KLOGG_USE_MIMALLOC
             << ", mimalloc v" << mi_version()
#endif
#ifdef KLOGG_USE_TBBMALLOC
             << ", tbbmalloc " << TBB_runtime_version()
#endif
             << ", default concurrency " << maxConcurrency;

    if ( maxConcurrency < 2 ) {
        maxConcurrency = 2;
        LOG_INFO << "Overriding default concurrency to " << maxConcurrency;
        tbb::global_control concurrencyControl( tbb::global_control::max_allowed_parallelism,
                                                maxConcurrency );
        QThreadPool::globalInstance()->setMaxThreadCount( static_cast<int>( maxConcurrency ) );
    }

    if ( !parameters.multi_instance && app.isSecondary() ) {
        LOG_INFO << "Found another klogg, pid " << app.primaryPid();
        app.sendFilesToPrimaryInstance( parameters.filenames );
    }
    else {
        StyleManager::applyStyle( config.style() );

        auto startNewSession = true;
        MainWindow* mw = nullptr;
        if ( parameters.load_session
             || ( parameters.filenames.empty() && !parameters.new_session
                  && config.loadLastSession() ) ) {
            mw = app.reloadSession();
            startNewSession = false;
        }
        else {
            mw = app.newWindow();
            mw->reloadGeometry();
            mw->show();
        }

        if ( parameters.window_width > 0 && parameters.window_height > 0 ) {
            mw->resize( parameters.window_width, parameters.window_height );
        }

        for ( const auto& filename : parameters.filenames ) {
            mw->loadInitialFile( filename, parameters.follow_file );
        }

        if ( startNewSession ) {
            app.clearInactiveSessions();
        }

        app.startBackgroundTasks();
    }

    return app.exec();
}
