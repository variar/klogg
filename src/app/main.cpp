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
#include <QtGlobal>
#include <QApplication>
#include <QThreadPool>

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <mimalloc.h>
#include <roaring.hh>

#ifdef KLOGG_HAS_HS
#include <hs.h>
#endif

#include "tbb/global_control.h"
#include "configuration.h"
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

namespace {
    // Constants for configuration
    constexpr int DEFAULT_MAX_CONCURRENCY = 2;

    void configureApplicationAttributes(bool enableQtHdpi, int scaleFactorRounding) {
        qputenv("QT_BEARER_POLL_TIMEOUT", QByteArray::number(std::numeric_limits<int>::max()));

        #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            #ifdef Q_OS_WIN
                QCoreApplication::setAttribute(Qt::AA_DisableWindowContextHelpButton);
            #endif

            if (!enableQtHdpi) {
                QCoreApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
            } else {
                #if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
                    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
                        static_cast<Qt::HighDpiScaleFactorRoundingPolicy>(scaleFactorRounding));
                #else
                    Q_UNUSED(scaleFactorRounding);
                #endif
                QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
                QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
            }
        #else
            Q_UNUSED(enableQtHdpi);
            Q_UNUSED(scaleFactorRounding);
        #endif

        QCoreApplication::setAttribute(Qt::AA_DontShowIconsInMenus);
    }

    void setupLogging(const CliParameters& parameters, const Configuration& config) {
        const auto logLevel = static_cast<logging::LogLevel>(
            std::max(parameters.log_level, config.loggingLevel()));
        
        logging::enableLogging(parameters.enable_logging || config.enableLogging(), logLevel);
        logging::enableFileLogging(parameters.log_to_file || config.enableLogging(), logLevel);
    }

    void initializeMemoryAllocators() {
        roaring_memory_t roaring_memory_allocators;
        roaring_memory_allocators.malloc = mi_malloc;
        roaring_memory_allocators.realloc = mi_realloc;
        roaring_memory_allocators.calloc = mi_calloc;
        roaring_memory_allocators.free = mi_free;
        roaring_memory_allocators.aligned_malloc = mi_aligned_alloc;
        roaring_memory_allocators.aligned_free = mi_free;
        roaring_init_memory_hook(roaring_memory_allocators);
    }

    void adjustMaxConcurrency(int& maxConcurrency) {
        if (maxConcurrency < DEFAULT_MAX_CONCURRENCY) {
            maxConcurrency = DEFAULT_MAX_CONCURRENCY;
            LOG_INFO << "Overriding default concurrency to " << maxConcurrency;
            tbb::global_control concurrencyControl(tbb::global_control::max_allowed_parallelism, maxConcurrency);
            QThreadPool::globalInstance()->setMaxThreadCount(maxConcurrency);
        }
    }
}

int main(int argc, char* argv[]) {
#ifdef KLOGG_USE_MIMALLOC
    mi_process_init();
#endif

    const auto& config = Configuration::getSynced();
    configureApplicationAttributes(config.enableQtHighDpi(), config.scaleFactorRounding());

    KloggApp app(argc, argv);
    MainWindow::installLanguage(config.language());
    CliParameters parameters(app);

    setupLogging(parameters, config);
    app.initCrashHandler();

    int maxConcurrency = tbb::global_control::active_value(tbb::global_control::max_allowed_parallelism);
    LOG_INFO << "Klogg instance"
             << ", mimalloc v" << mi_version()
             << ", default concurrency " << maxConcurrency;

    initializeMemoryAllocators();

#ifdef KLOGG_HAS_HS
    hs_set_allocator(mi_malloc, mi_free);
#endif

    adjustMaxConcurrency(maxConcurrency);

    if (!parameters.multi_instance && app.isSecondary()) {
        LOG_INFO << "Found another klogg, pid " << app.primaryPid();
        app.sendFilesToPrimaryInstance(parameters.filenames);
    } else {
        StyleManager::applyStyle(config.style());

        MainWindow* mw = nullptr;
        bool startNewSession = true;

        if (parameters.load_session || (parameters.filenames.empty() && !parameters.new_session && config.loadLastSession())) {
            mw = app.reloadSession();
            startNewSession = false;
        } else {
            mw = app.newWindow();
            mw->reloadGeometry();
            mw->show();
        }

        if (parameters.window_width > 0 && parameters.window_height > 0) {
            mw->resize(parameters.window_width, parameters.window_height);
        }

        for (const auto& filename : parameters.filenames) {
            mw->loadInitialFile(filename, parameters.follow_file);
        }

        if (startNewSession) {
            app.clearInactiveSessions();
        }

        app.startBackgroundTasks();
    }

    return app.exec();
}

