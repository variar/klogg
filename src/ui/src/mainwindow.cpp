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

// This file implements MainWindow. It is responsible for creating and
// managing the menus, the toolbar, and the CrawlerWidget. It also
// load/save the settings on opening/closing of the app

#include <QNetworkReply>
#include <cassert>
#include <iostream>

#include <cmath>

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // Q_OS_WIN

#include <QAction>
#include <QClipboard>
#include <QCloseEvent>
#include <QDesktopWidget>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QListView>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QProgressDialog>
#include <QScreen>
#include <QSortFilterProxyModel>
#include <QStringListModel>
#include <QTemporaryFile>
#include <QTextBrowser>
#include <QToolBar>
#include <QUrl>
#include <QUrlQuery>
#include <QWindow>

#include "mainwindow.h"

#include "crawlerwidget.h"
#include "decompressor.h"
#include "downloader.h"
#include "encodings.h"
#include "favoritefiles.h"
#include "highlightersdialog.h"
#include "highlightersmenu.h"
#include "klogg_version.h"
#include "log.h"
#include "openfilehelper.h"
#include "optionsdialog.h"
#include "predefinedfilters.h"
#include "predefinedfiltersdialog.h"
#include "readablesize.h"
#include "recentfiles.h"
#include "styles.h"
#include "tabbedcrawlerwidget.h"

namespace {

void signalCrawlerToFollowFile( CrawlerWidget* crawler_widget )
{
    QMetaObject::invokeMethod( crawler_widget, "followSet", Q_ARG( bool, true ) );
}

} // namespace

MainWindow::MainWindow( WindowSession session )
    : session_( std::move( session ) )
    , mainIcon_()
    , iconLoader_( this )
    , signalMux_()
    , quickFindMux_( session_.getQuickFindPattern() )
    , mainTabWidget_()
    , tempDir_( QDir::temp().filePath( "klogg_temp_" ) )
{
    createActions();
    createMenus();
    createToolBars();

    setAcceptDrops( true );

    // Default geometry
    const QRect geometry = QApplication::desktop()->availableGeometry( this );
    setGeometry( geometry.x() + 20, geometry.y() + 40, geometry.width() - 140,
                 geometry.height() - 140 );

    mainIcon_.addFile( ":/images/hicolor/16x16/klogg.png" );
    // mainIcon_.addFile( ":/images/hicolor/24x24/klogg.png" );
    mainIcon_.addFile( ":/images/hicolor/32x32/klogg.png" );
    mainIcon_.addFile( ":/images/hicolor/48x48/klogg.png" );

    setWindowIcon( mainIcon_ );
    readSettings();

    createTrayIcon();

    // Connect the signals to the mux (they will be forwarded to the
    // "current" crawlerwidget

    // Send actions to the crawlerwidget
    signalMux_.connect( this, SIGNAL( followSet( bool ) ), SIGNAL( followSet( bool ) ) );
    signalMux_.connect( this, SIGNAL( optionsChanged() ), SLOT( applyConfiguration() ) );
    signalMux_.connect( this, SIGNAL( enteringQuickFind() ), SLOT( enteringQuickFind() ) );
    signalMux_.connect( &quickFindWidget_, SIGNAL( close() ), SLOT( exitingQuickFind() ) );

    // Actions from the CrawlerWidget
    signalMux_.connect( SIGNAL( followModeChanged( bool ) ), this,
                        SLOT( changeFollowMode( bool ) ) );
    signalMux_.connect( SIGNAL( updateLineNumber( LineNumber ) ), this,
                        SLOT( lineNumberHandler( LineNumber ) ) );

    // Register for progress status bar
    signalMux_.connect( SIGNAL( loadingProgressed( int ) ), this,
                        SLOT( updateLoadingProgress( int ) ) );
    signalMux_.connect( SIGNAL( loadingFinished( LoadingStatus ) ), this,
                        SLOT( handleLoadingFinished( LoadingStatus ) ) );

    // Register for checkbox changes
    signalMux_.connect( SIGNAL( searchRefreshChanged( bool ) ), this,
                        SLOT( handleSearchRefreshChanged( bool ) ) );
    signalMux_.connect( SIGNAL( matchCaseChanged( bool ) ), this,
                        SLOT( handleMatchCaseChanged( bool ) ) );

    // Configure the main tabbed widget
    mainTabWidget_.setDocumentMode( true );
    mainTabWidget_.setMovable( true );
    // mainTabWidget_.setTabShape( QTabWidget::Triangular );
    mainTabWidget_.setTabsClosable( true );

    scratchPad_.setWindowIcon( mainIcon_ );
    scratchPad_.setWindowTitle( "klogg - scratchpad" );

    connect( &mainTabWidget_, &TabbedCrawlerWidget::tabCloseRequested, this,
             QOverload<int>::of( &MainWindow::closeTab ) );
    connect( &mainTabWidget_, &TabbedCrawlerWidget::currentChanged, this,
             &MainWindow::currentTabChanged );

    // Establish the QuickFindWidget and mux ( to send requests from the
    // QFWidget to the right window )
    connect( &quickFindWidget_, SIGNAL( patternConfirmed( const QString&, bool ) ), &quickFindMux_,
             SLOT( confirmPattern( const QString&, bool ) ) );
    connect( &quickFindWidget_, SIGNAL( patternUpdated( const QString&, bool ) ), &quickFindMux_,
             SLOT( setNewPattern( const QString&, bool ) ) );
    connect( &quickFindWidget_, SIGNAL( cancelSearch() ), &quickFindMux_, SLOT( cancelSearch() ) );
    connect( &quickFindWidget_, SIGNAL( searchForward() ), &quickFindMux_,
             SLOT( searchForward() ) );
    connect( &quickFindWidget_, SIGNAL( searchBackward() ), &quickFindMux_,
             SLOT( searchBackward() ) );
    connect( &quickFindWidget_, SIGNAL( searchNext() ), &quickFindMux_, SLOT( searchNext() ) );

    // QuickFind changes coming from the views
    connect( &quickFindMux_, SIGNAL( patternChanged( const QString& ) ), this,
             SLOT( changeQFPattern( const QString& ) ) );
    connect( &quickFindMux_, SIGNAL( notify( const QFNotification& ) ), &quickFindWidget_,
             SLOT( notify( const QFNotification& ) ) );
    connect( &quickFindMux_, SIGNAL( clearNotification() ), &quickFindWidget_,
             SLOT( clearNotification() ) );

    // Construct the QuickFind bar
    quickFindWidget_.hide();

    QWidget* central_widget = new QWidget();
    auto* main_layout = new QVBoxLayout();
    main_layout->setContentsMargins( 0, 0, 0, 0 );
    main_layout->addWidget( &mainTabWidget_ );
    main_layout->addWidget( &quickFindWidget_ );
    central_widget->setLayout( main_layout );

    setCentralWidget( central_widget );

    auto clipboard = QGuiApplication::clipboard();
    connect( clipboard, &QClipboard::dataChanged, this, &MainWindow::onClipboardDataChanged );
    onClipboardDataChanged();

    updateTitleBar( "" );

    applyStyle();
}

void MainWindow::reloadGeometry()
{
    QByteArray geometry;

    session_.restoreGeometry( &geometry );
    restoreGeometry( geometry );
}

void MainWindow::reloadSession()
{
    const auto& config = Configuration::get();
    const auto followFileOnLoad = config.followFileOnLoad() && config.anyFileWatchEnabled();

    int current_file_index = -1;
    const auto openedFiles
        = session_.restore( [] { return new CrawlerWidget(); }, &current_file_index );

    for ( const auto& open_file : openedFiles ) {
        QString file_name = { open_file.first };
        auto* crawler_widget = static_cast<CrawlerWidget*>( open_file.second );

        if ( crawler_widget ) {
            mainTabWidget_.addCrawler( crawler_widget, file_name );

            if ( followFileOnLoad ) {
                signalCrawlerToFollowFile( crawler_widget );
            }
        }
    }

    if ( current_file_index >= 0 ) {
        mainTabWidget_.setCurrentIndex( current_file_index );

        if ( followFileOnLoad ) {
            followAction->setChecked( true );
        }
    }

    updateOpenedFilesMenu();
}

void MainWindow::loadInitialFile( QString fileName, bool followFile )
{
    LOG( logDEBUG ) << "loadInitialFile";

    // Is there a file passed as argument?
    if ( !fileName.isEmpty() ) {
        loadFile( fileName, followFile );
    }
}

// Menu actions
void MainWindow::createActions()
{
    const auto& config = Configuration::get();

    newWindowAction = new QAction( tr( "&New window" ), this );
    newWindowAction->setShortcut( QKeySequence::New );
    newWindowAction->setStatusTip( tr( "Create new klogg window" ) );
    connect( newWindowAction, &QAction::triggered, [=] { emit newWindow(); } );
    newWindowAction->setVisible( config.allowMultipleWindows() );

    openAction = new QAction( tr( "&Open..." ), this );
    openAction->setShortcuts( QKeySequence::keyBindings( QKeySequence::Open ) );
    openAction->setStatusTip( tr( "Open a file" ) );
    connect( openAction, &QAction::triggered, [this]( auto ) { this->open(); } );

    closeAction = new QAction( tr( "&Close" ), this );
    closeAction->setShortcuts( QKeySequence::keyBindings( QKeySequence::Close ) );
    closeAction->setStatusTip( tr( "Close document" ) );
    connect( closeAction, &QAction::triggered, [this]( auto ) { this->closeTab(); } );

    closeAllAction = new QAction( tr( "Close &All" ), this );
    closeAllAction->setStatusTip( tr( "Close all documents" ) );
    connect( closeAllAction, &QAction::triggered, [this]( auto ) { this->closeAll(); } );

    recentFilesGroup = new QActionGroup( this );
    connect( recentFilesGroup, &QActionGroup::triggered, this, &MainWindow::openFileFromRecent );
    for ( auto i = 0u; i < recentFileActions.size(); ++i ) {
        recentFileActions[ i ] = new QAction( this );
        recentFileActions[ i ]->setVisible( false );
        recentFileActions[ i ]->setActionGroup( recentFilesGroup );
    }

    exitAction = new QAction( tr( "E&xit" ), this );
    exitAction->setShortcut( tr( "Ctrl+Q" ) );
    exitAction->setStatusTip( tr( "Exit the application" ) );
    connect( exitAction, &QAction::triggered, this, &MainWindow::exitRequested );

    copyAction = new QAction( tr( "&Copy" ), this );
    copyAction->setShortcuts( QKeySequence::keyBindings( QKeySequence::Copy ) );
    copyAction->setStatusTip( tr( "Copy the selection" ) );
    connect( copyAction, &QAction::triggered, [this]( auto ) { this->copy(); } );

    selectAllAction = new QAction( tr( "Select &All" ), this );
    selectAllAction->setShortcut( tr( "Ctrl+A" ) );
    selectAllAction->setStatusTip( tr( "Select all the text" ) );
    connect( selectAllAction, &QAction::triggered, [this]( auto ) { this->selectAll(); } );

    findAction = new QAction( tr( "&Find..." ), this );
    findAction->setShortcut( QKeySequence::Find );
    findAction->setStatusTip( tr( "Find the text" ) );
    connect( findAction, &QAction::triggered, [this]( auto ) { this->find(); } );

    clearLogAction = new QAction( tr( "Clear file..." ), this );
    clearLogAction->setStatusTip( tr( "Clear current file" ) );
    clearLogAction->setShortcuts( QKeySequence::Cut );
    connect( clearLogAction, &QAction::triggered, [this]( auto ) { this->clearLog(); } );

    openContainingFolderAction = new QAction( tr( "Open containing folder" ), this );
    openContainingFolderAction->setStatusTip( tr( "Open folder containing current file" ) );
    connect( openContainingFolderAction, &QAction::triggered,
             [this]( auto ) { this->openContainingFolder(); } );

    openInEditorAction = new QAction( tr( "Open in editor" ), this );
    openInEditorAction->setStatusTip( tr( "Open current file in default editor" ) );
    connect( openInEditorAction, &QAction::triggered, [this]( auto ) { this->openInEditor(); } );

    copyPathToClipboardAction = new QAction( tr( "Copy full path" ), this );
    copyPathToClipboardAction->setStatusTip( tr( "Copy full path for file to clipboard" ) );
    connect( copyPathToClipboardAction, &QAction::triggered,
             [this]( auto ) { this->copyFullPath(); } );

    openClipboardAction = new QAction( tr( "Open from clipboard" ), this );
    openClipboardAction->setStatusTip( tr( "Open clipboard as log file" ) );
    openClipboardAction->setShortcuts( QKeySequence::keyBindings( QKeySequence::Paste ) );
    connect( openClipboardAction, &QAction::triggered, [this]( auto ) { this->openClipboard(); } );

    openUrlAction = new QAction( tr( "Open from URL..." ), this );
    openUrlAction->setStatusTip( tr( "Open URL as log file" ) );
    connect( openUrlAction, &QAction::triggered, [this]( auto ) { this->openUrl(); } );

    overviewVisibleAction = new QAction( tr( "Matches &overview" ), this );
    overviewVisibleAction->setCheckable( true );
    overviewVisibleAction->setChecked( config.isOverviewVisible() );
    connect( overviewVisibleAction, &QAction::toggled, this,
             &MainWindow::toggleOverviewVisibility );

    lineNumbersVisibleInMainAction = new QAction( tr( "Line &numbers in main view" ), this );
    lineNumbersVisibleInMainAction->setCheckable( true );
    lineNumbersVisibleInMainAction->setChecked( config.mainLineNumbersVisible() );
    connect( lineNumbersVisibleInMainAction, &QAction::toggled, this,
             &MainWindow::toggleMainLineNumbersVisibility );

    lineNumbersVisibleInFilteredAction
        = new QAction( tr( "Line &numbers in filtered view" ), this );
    lineNumbersVisibleInFilteredAction->setCheckable( true );
    lineNumbersVisibleInFilteredAction->setChecked( config.filteredLineNumbersVisible() );
    connect( lineNumbersVisibleInFilteredAction, &QAction::toggled, this,
             &MainWindow::toggleFilteredLineNumbersVisibility );

    followAction = new QAction( tr( "&Follow File" ), this );

    followAction->setShortcuts( QList<QKeySequence>()
                                << QKeySequence( Qt::Key_F ) << QKeySequence( Qt::Key_F10 ) );

    followAction->setCheckable( true );
    followAction->setEnabled( config.anyFileWatchEnabled() );
    connect( followAction, &QAction::toggled, this, &MainWindow::followSet );

    reloadAction = new QAction( tr( "&Reload" ), this );
    reloadAction->setShortcuts( QKeySequence::keyBindings( QKeySequence::Refresh ) );
    signalMux_.connect( reloadAction, SIGNAL( triggered() ), SLOT( reload() ) );

    stopAction = new QAction( tr( "&Stop" ), this );
    stopAction->setEnabled( true );
    signalMux_.connect( stopAction, SIGNAL( triggered() ), SLOT( stopLoading() ) );

    editHighlightersAction = new QAction( tr( "Configure &highlighters..." ), this );
    editHighlightersAction->setStatusTip( tr( "Show highlighters configuration" ) );
    connect( editHighlightersAction, &QAction::triggered,
             [this]( auto ) { this->editHighlighters(); } );

    optionsAction = new QAction( tr( "&Options..." ), this );
    optionsAction->setStatusTip( tr( "Show the Options box" ) );
    connect( optionsAction, &QAction::triggered, [this]( auto ) { this->options(); } );

    showDocumentationAction = new QAction( tr( "&Documentation..." ), this );
    showDocumentationAction->setStatusTip( tr( "Show documentation" ) );
    connect( showDocumentationAction, &QAction::triggered,
             [this]( auto ) { this->documentation(); } );

    aboutAction = new QAction( tr( "&About" ), this );
    aboutAction->setStatusTip( tr( "Show the About box" ) );
    connect( aboutAction, &QAction::triggered, [this]( auto ) { this->about(); } );

    aboutQtAction = new QAction( tr( "About &Qt" ), this );
    aboutQtAction->setStatusTip( tr( "Show the Qt library's About box" ) );
    connect( aboutQtAction, &QAction::triggered, [this]( auto ) { this->aboutQt(); } );

    reportIssueAction = new QAction( tr( "Report issue..." ), this );
    reportIssueAction->setStatusTip( tr( "Report an issue on GitHub" ) );
    connect( reportIssueAction, &QAction::triggered, [this]( auto ) { this->reportIssue(); } );

    generateDumpAction = new QAction( tr( "Generate crash dump" ), this );
    generateDumpAction->setStatusTip( tr( "Generate diagnostic crash dump" ) );
    connect( generateDumpAction, &QAction::triggered, [this]( auto ) { this->generateDump(); } );

    showScratchPadAction = new QAction( tr( "Scratchpad" ), this );
    showScratchPadAction->setStatusTip( tr( "Show the scratchpad" ) );
    connect( showScratchPadAction, &QAction::triggered,
             [this]( auto ) { this->showScratchPad(); } );

    encodingGroup = new QActionGroup( this );
    connect( encodingGroup, &QActionGroup::triggered, this, &MainWindow::encodingChanged );

    favoritesGroup = new QActionGroup( this );
    connect( favoritesGroup, &QActionGroup::triggered, this, &MainWindow::openFileFromFavorites );

    openedFilesGroup = new QActionGroup( this );
    connect( openedFilesGroup, &QActionGroup::triggered, this, &MainWindow::switchToOpenedFile );

    addToFavoritesAction = new QAction( tr( "Add to favorites" ), this );
    addToFavoritesAction->setData( true );
    connect( addToFavoritesAction, &QAction::triggered,
             [this]( auto ) { this->addToFavorites(); } );

    addToFavoritesMenuAction = new QAction( tr( "Add to favorites" ), this );
    connect( addToFavoritesMenuAction, &QAction::triggered,
             [this]( auto ) { this->addToFavorites(); } );

    removeFromFavoritesAction = new QAction( tr( "Remove from favorites..." ), this );
    connect( removeFromFavoritesAction, &QAction::triggered,
             [this]( auto ) { this->removeFromFavorites(); } );

    selectOpenFileAction = new QAction( tr( "Switch to opened file..." ), this );
    connect( selectOpenFileAction, &QAction::triggered,
             [this]( auto ) { this->selectOpenedFile(); } );
    selectOpenFileAction->setShortcuts( QList<QKeySequence>()
                                        << QKeySequence( Qt::SHIFT | Qt::CTRL | Qt::Key_O ) );

    predefinedFiltersDialogAction = new QAction( tr( "Configure predefined filters" ), this );
    predefinedFiltersDialogAction->setStatusTip( tr( "Show dialog to configure filters" ) );
    connect( predefinedFiltersDialogAction, &QAction::triggered,
             [ this ]( auto ) { this->editPredefinedFilters(); } );
}

void MainWindow::loadIcons()
{
    openAction->setIcon( iconLoader_.load( "icons8-add-file" ) );
    stopAction->setIcon( iconLoader_.load( "icons8-delete" ) );
    reloadAction->setIcon( iconLoader_.load( "icons8-restore-page" ) );
    followAction->setIcon( iconLoader_.load( "icons8-fast-forward" ) );
    showScratchPadAction->setIcon( iconLoader_.load( "icons8-create" ) );
    addToFavoritesAction->setIcon( iconLoader_.load( "icons8-star" ) );
    addToFavoritesMenuAction->setIcon( iconLoader_.load( "icons8-star" ) );
}

void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu( tr( "&File" ) );
    fileMenu->setToolTipsVisible( true );
    fileMenu->addAction( newWindowAction );
    fileMenu->addAction( openAction );
    fileMenu->addAction( openClipboardAction );
    fileMenu->addAction( openUrlAction );
    fileMenu->addAction( closeAction );
    fileMenu->addAction( closeAllAction );
    fileMenu->addSeparator();

    for ( auto i = 0u; i < recentFileActions.size(); ++i ) {
        fileMenu->addAction( recentFileActions[ i ] );
    }

    fileMenu->addSeparator();
    fileMenu->addAction( exitAction );

    editMenu = menuBar()->addMenu( tr( "&Edit" ) );
    editMenu->addAction( copyAction );
    editMenu->addAction( selectAllAction );
    editMenu->addSeparator();
    editMenu->addAction( findAction );
    editMenu->addSeparator();
    editMenu->addAction( copyPathToClipboardAction );
    editMenu->addAction( openContainingFolderAction );
    editMenu->addSeparator();
    editMenu->addAction( openInEditorAction );
    editMenu->addAction( clearLogAction );
    editMenu->setEnabled( false );

    viewMenu = menuBar()->addMenu( tr( "&View" ) );
    openedFilesMenu = viewMenu->addMenu( "Opened files" );
    viewMenu->addSeparator();
    viewMenu->addAction( overviewVisibleAction );
    viewMenu->addSeparator();
    viewMenu->addAction( lineNumbersVisibleInMainAction );
    viewMenu->addAction( lineNumbersVisibleInFilteredAction );
    viewMenu->addSeparator();
    viewMenu->addAction( followAction );
    viewMenu->addSeparator();
    viewMenu->addAction( reloadAction );

    toolsMenu = menuBar()->addMenu( tr( "&Tools" ) );

    highlightersMenu = toolsMenu->addMenu( "Highlighters" );
    connect( highlightersMenu, &QMenu::aboutToShow,
             [this]() { setCurrentHighlighterAction( highlightersActionGroup ); } );

    toolsMenu->addAction( predefinedFiltersDialogAction );

    toolsMenu->addSeparator();
    toolsMenu->addAction( optionsAction );

    toolsMenu->addSeparator();
    toolsMenu->addAction( showScratchPadAction );

    menuBar()->addMenu( EncodingMenu::generate( encodingGroup ) );
    menuBar()->addSeparator();

    favoritesMenu = menuBar()->addMenu( tr( "F&avorites" ) );
    favoritesMenu->setToolTipsVisible( true );

    helpMenu = menuBar()->addMenu( tr( "&Help" ) );
    helpMenu->addAction( showDocumentationAction );
    helpMenu->addSeparator();
    helpMenu->addAction( reportIssueAction );
    helpMenu->addAction( generateDumpAction );
    helpMenu->addSeparator();
    helpMenu->addAction( aboutQtAction );
    helpMenu->addAction( aboutAction );
}

void MainWindow::createToolBars()
{
    infoLine = new PathLine();
    infoLine->setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
    infoLine->setLineWidth( 0 );
    infoLine->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Minimum );

    sizeField = new QLabel();
    sizeField->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );

    dateField = new QLabel();
    dateField->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );

    encodingField = new QLabel();
    dateField->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );

    lineNbField = new QLabel();
    lineNbField->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
    lineNbField->setContentsMargins( 2, 0, 2, 0 );

    toolBar = addToolBar( tr( "&Toolbar" ) );
    toolBar->setIconSize( QSize( 16, 16 ) );
    toolBar->setMovable( false );
    toolBar->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Minimum );
    toolBar->addAction( openAction );
    toolBar->addAction( reloadAction );
    toolBar->addAction( followAction );
    toolBar->addAction( addToFavoritesAction );
    toolBar->addWidget( infoLine );
    toolBar->addAction( stopAction );

    infoToolbarSeparators.push_back( toolBar->addSeparator() );
    toolBar->addWidget( sizeField );
    infoToolbarSeparators.push_back( toolBar->addSeparator() );
    toolBar->addWidget( dateField );
    infoToolbarSeparators.push_back( toolBar->addSeparator() );
    toolBar->addWidget( encodingField );
    infoToolbarSeparators.push_back( toolBar->addSeparator() );
    toolBar->addWidget( lineNbField );
    infoToolbarSeparators.push_back( toolBar->addSeparator() );
    toolBar->addAction( showScratchPadAction );

    showInfoLabels( false );
}

void MainWindow::applyStyle()
{
    const auto& config = Configuration::get();
    auto style = config.style();
    LOG( logINFO ) << "Setting style to " << style;
    if ( style == DarkStyleKey ) {
        QFile styleFile( ":qdarkstyle/style.qss" );
        styleFile.open( QFile::ReadOnly | QFile::Text );
        QTextStream styleSream( &styleFile );
        QApplication::setStyle( availableStyles().front() );
        qApp->setStyleSheet( styleSream.readAll() );
    }
    else {
        QApplication::setStyle( style );
        qApp->setStyleSheet( "" );
    }
}

void MainWindow::createTrayIcon()
{
    trayIcon_ = new QSystemTrayIcon( this );

    QMenu* trayMenu = new QMenu( this );
    QAction* openWindowAction = new QAction( tr( "Open window" ), this );
    QAction* quitAction = new QAction( tr( "Quit" ), this );

    trayMenu->addAction( openWindowAction );
    trayMenu->addAction( quitAction );

    connect( openWindowAction, &QAction::triggered, this, &QMainWindow::show );
    connect( quitAction, &QAction::triggered, [this] {
        this->isCloseFromTray_ = true;
        this->close();
    } );

    trayIcon_->setIcon( mainIcon_ );
    trayIcon_->setToolTip( "klogg log viewer" );
    trayIcon_->setContextMenu( trayMenu );

    connect( trayIcon_, &QSystemTrayIcon::activated,
             [this]( QSystemTrayIcon::ActivationReason reason ) {
                 switch ( reason ) {
                 case QSystemTrayIcon::Trigger:
                     if ( !this->isVisible() ) {
                         this->show();
                     }
                     else {
                         this->hide();
                     }
                     break;
                 default:
                     break;
                 }
             } );

    if ( Configuration::get().minimizeToTray() ) {
        trayIcon_->show();
    }
}
//
// Slots
//

// Opens the file selection dialog to select a new log file
void MainWindow::open()
{
    QString defaultDir = ".";

    // Default to the path of the current file if there is one
    if ( auto current = currentCrawlerWidget() ) {
        QString current_file = session_.getFilename( current );
        QFileInfo fileInfo = QFileInfo( current_file );
        defaultDir = fileInfo.path();
    }

    const auto selectedFiles = QFileDialog::getOpenFileUrls(
        this, tr( "Open file" ), QUrl::fromLocalFile( defaultDir ), tr( "All files (*)" ) );

    std::vector<QUrl> localFiles;
    std::vector<QUrl> remoteFiles;

    std::partition_copy( selectedFiles.cbegin(), selectedFiles.cend(),
                         std::back_inserter( localFiles ), std::back_inserter( remoteFiles ),
                         []( const QUrl& url ) { return url.isLocalFile(); } );

    for ( const auto& localFile : localFiles ) {
        loadFile( localFile.toLocalFile() );
    }

    for ( const auto& remoteFile : remoteFiles ) {
        openRemoteFile( remoteFile );
    }
}

void MainWindow::openRemoteFile( const QUrl& url )
{
    Downloader downloader;

    QProgressDialog progressDialog;
    progressDialog.setLabelText( QString( "Downloading %1" ).arg( url.toString() ) );

    connect( &downloader, &Downloader::downloadProgress,
             [&progressDialog]( qint64 bytesReceived, qint64 bytesTotal ) {
                 const auto progress = calculateProgress( bytesReceived, bytesTotal );
                 progressDialog.setRange( 0, 100 );
                 progressDialog.setValue( progress );
             } );

    connect( &downloader, &Downloader::finished,
             [&progressDialog]( bool isOk ) { progressDialog.done( isOk ? 0 : 1 ); } );

    auto tempFile = new QTemporaryFile( tempDir_.filePath( url.fileName() ), this );
    if ( tempFile->open() ) {
        downloader.download( url, tempFile );
        if ( !progressDialog.exec() ) {
            loadFile( tempFile->fileName() );
        }
        else {
            QMessageBox::critical( this, "Klogg - File download", downloader.lastError() );
        }
    }
    else {
        QMessageBox::critical( this, "Klogg - File download", "Failed to create temp file" );
    }
}

void MainWindow::switchToOpenedFile( QAction* action )
{
    if ( !action ) {
        return;
    }

    loadFile( action->data().toString() );
}

void MainWindow::openFileFromRecent( QAction* action )
{
    if ( !action ) {
        return;
    }

    const auto filename = action->data().toString();
    if ( QFileInfo{ filename }.isReadable() ) {
        loadFile( filename );
    }
    else {
        const auto userAction = QMessageBox::question(
            this, "klogg - remove from recent",
            QString( "Could not read file %1. Remove it from recent files?" ).arg( filename ),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No );

        if ( userAction == QMessageBox::Yes ) {
            removeFromRecent( filename );
        }
    }
}

void MainWindow::openFileFromFavorites( QAction* action )
{
    if ( !action ) {
        return;
    }

    const auto filename = action->data().toString();
    if ( QFileInfo{ filename }.isReadable() ) {
        loadFile( filename );
    }
    else {
        const auto userAction = QMessageBox::question(
            this, "klogg - remove from favorites",
            QString( "Could not read file %1. Remove it from favorites?" ).arg( filename ),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No );

        if ( userAction == QMessageBox::Yes ) {
            removeFromFavorites( filename );
        }
    }
}

// Close current tab
void MainWindow::closeTab()
{
    int currentIndex = mainTabWidget_.currentIndex();

    if ( currentIndex >= 0 ) {
        closeTab( currentIndex );
    }
    else {
        this->close();
    }
}

// Close all tabs
void MainWindow::closeAll()
{
    while ( mainTabWidget_.count() ) {
        closeTab( 0 );
    }
}

// Select all the text in the currently selected view
void MainWindow::selectAll()
{
    CrawlerWidget* current = currentCrawlerWidget();

    if ( current )
        current->selectAll();
}

// Copy the currently selected line into the clipboard
void MainWindow::copy()
{
    static QClipboard* clipboard = QApplication::clipboard();

    if ( auto current = currentCrawlerWidget() ) {
        clipboard->setText( current->getSelectedText() );

        // Put it in the global selection as well (X11 only)
        clipboard->setText( current->getSelectedText(), QClipboard::Selection );
    }
}

// Display the QuickFind bar
void MainWindow::find()
{
    displayQuickFindBar( QuickFindMux::Forward );
}

void MainWindow::clearLog()
{
    const auto current_file = session_.getFilename( currentCrawlerWidget() );
    if ( QMessageBox::question( this, "klogg - clear file",
                                QString( "Clear file %1?" ).arg( current_file ) )
         == QMessageBox::Yes ) {
        QFile::resize( current_file, 0 );
    }
}

void MainWindow::copyFullPath()
{
    const auto current_file = session_.getFilename( currentCrawlerWidget() );
    QApplication::clipboard()->setText( QDir::toNativeSeparators( current_file ) );
}

void MainWindow::openContainingFolder()
{
    showPathInFileExplorer( session_.getFilename( currentCrawlerWidget() ) );
}

void MainWindow::openInEditor()
{
    openFileInDefaultApplication( session_.getFilename( currentCrawlerWidget() ) );
}

void MainWindow::onClipboardDataChanged()
{
    auto clipboard = QGuiApplication::clipboard();
    QString subtype;
    auto text = clipboard->text( subtype );

    LOG( logINFO ) << "Clipboard data type: " << subtype;

    openClipboardAction->setEnabled( !text.isEmpty() );
}

void MainWindow::openClipboard()
{
    auto clipboard = QGuiApplication::clipboard();
    QString subtype;
    auto text = clipboard->text( subtype );
    if ( text.isEmpty() ) {
        return;
    }

    auto tempFile = new QTemporaryFile( tempDir_.filePath( "klogg_clipboard" ), this );
    if ( tempFile->open() ) {
        tempFile->write( text.toUtf8() );
        tempFile->flush();

        loadFile( tempFile->fileName() );
    }
}

void MainWindow::openUrl()
{
    bool ok;
    const auto urlInClipboard = QUrl::fromUserInput( QApplication::clipboard()->text() );
    const auto selectedUrl = urlInClipboard.isValid() ? urlInClipboard.toString() : QString{};

    QString url
        = QInputDialog::getText( this, tr( "Open URL as log file" ), tr( "URL to download:" ),
                                 QLineEdit::Normal, selectedUrl, &ok );
    if ( ok && !url.isEmpty() ) {
        openRemoteFile( url );
    }
}

// Opens the 'Highlighters' dialog box
void MainWindow::editHighlighters()
{
    HighlightersDialog dialog( this );
    signalMux_.connect( &dialog, SIGNAL( optionsChanged() ), SLOT( applyConfiguration() ) );

    connect( &dialog, &HighlightersDialog::optionsChanged, [this]() { updateHighlightersMenu(); } );

    dialog.exec();
    signalMux_.disconnect( &dialog, SIGNAL( optionsChanged() ), SLOT( applyConfiguration() ) );
}

// Opens dialog to configure predefined filters
void MainWindow::editPredefinedFilters()
{
    PredefinedFiltersDialog dialog( this );

    signalMux_.connect( &dialog, SIGNAL( optionsChanged() ), SLOT( applyConfiguration() ) );

    dialog.exec();
    signalMux_.disconnect( &dialog, SIGNAL( optionsChanged() ), SLOT( applyConfiguration() ) );
}

// Opens the 'Options' modal dialog box
void MainWindow::options()
{
    OptionsDialog dialog( this );
    signalMux_.connect( &dialog, SIGNAL( optionsChanged() ), SLOT( applyConfiguration() ) );

    connect( &dialog, &OptionsDialog::optionsChanged, [this]() {
        const auto& config = Configuration::get();
        plog::EnableLogging( config.enableLogging(), config.loggingLevel() );

        newWindowAction->setVisible( config.allowMultipleWindows() );
        followAction->setEnabled( config.anyFileWatchEnabled() );

        applyStyle();
    } );
    dialog.exec();

    signalMux_.disconnect( &dialog, SIGNAL( optionsChanged() ), SLOT( applyConfiguration() ) );
}

void MainWindow::about()
{
    QMessageBox::about(
        this, tr( "About klogg" ),
        QString(
            "<h2>klogg %1</h2>"
            "<p>A fast, advanced log explorer.</p>"
            "<p>Built %2 from %3</p>"
            "<p><a href=\"https://github.com/variar/klogg\">https://github.com/variar/klogg</a></p>"
            "<p>This is fork of glogg</p>"
            "<p><a href=\"http://glogg.bonnefon.org/\">http://glogg.bonnefon.org/</a></p>"
            "<p>Using icons from <a href=\"https://icons8.com\">icons8.com</a> project</p>"
            "<p>Copyright &copy; 2020 Nicolas Bonnefon, Anton Filimonov and other contributors</p>"
            "<p>You may modify and redistribute the program under the terms of the GPL (version 3 "
            "or later).</p>" )
            .arg( kloggVersion(), kloggBuildDate(), kloggCommit() ) );
}

void MainWindow::aboutQt()
{
    QMessageBox::aboutQt( this, tr( "About Qt" ) );
}

void MainWindow::documentation()
{
    QFile doc( ":/documentation.html" );
    if ( doc.open( QIODevice::ReadOnly | QIODevice::Text ) ) {
        const auto text = QString::fromUtf8( doc.readAll() );
        QTextBrowser* tb = new QTextBrowser();
        tb->setOpenExternalLinks( true );
        tb->setHtml( text );
        tb->setWindowFlags( Qt::Window );
        tb->setAttribute( Qt::WA_DeleteOnClose );
        tb->setWindowTitle( "klogg documentation" );
        tb->resize( this->width() / 2, this->height() );
        tb->show();
    }
    else {
        LOG( logERROR ) << "Can't open documentation resource";
    }
}

void MainWindow::showScratchPad()
{
    auto state = scratchPad_.windowState();
    state.setFlag( Qt::WindowMinimized, false );
    scratchPad_.setWindowState( state );

    scratchPad_.show();
    scratchPad_.activateWindow();
}

void MainWindow::encodingChanged( QAction* action )
{
    const auto mibData = action->data();
    absl::optional<int> mib;
    if ( mibData.isValid() ) {
        mib = mibData.toInt();
    }

    LOG( logDEBUG ) << "encodingChanged, encoding " << mib;
    if ( auto crawler = currentCrawlerWidget() ) {
        crawler->setEncoding( mib );
        updateInfoLine();
    }
}

void MainWindow::toggleOverviewVisibility( bool isVisible )
{
    auto& config = Configuration::get();
    config.setOverviewVisible( isVisible );
    emit optionsChanged();
}

void MainWindow::toggleMainLineNumbersVisibility( bool isVisible )
{
    auto& config = Configuration::get();

    config.setMainLineNumbersVisible( isVisible );
    emit optionsChanged();
}

void MainWindow::toggleFilteredLineNumbersVisibility( bool isVisible )
{
    auto& config = Configuration::get();

    config.setFilteredLineNumbersVisible( isVisible );
    emit optionsChanged();
}

void MainWindow::changeFollowMode( bool follow )
{
    auto& config = Configuration::get();
    if ( follow && !( config.nativeFileWatchEnabled() || config.pollingEnabled() ) ) {
        LOG( logWARNING ) << "File watch disabled in settings";
    }

    followAction->setChecked( follow );
}

void MainWindow::lineNumberHandler( LineNumber line )
{
    // The line number received is the internal (starts at 0)
    uint64_t fileSize{};
    uint32_t fileNbLine{};
    QDateTime lastModified;

    session_.getFileInfo( currentCrawlerWidget(), &fileSize, &fileNbLine, &lastModified );

    if ( fileNbLine != 0 ) {
        lineNbField->setText( tr( "Line %1/%2" ).arg( line.get() + 1 ).arg( fileNbLine ) );
    }
    else {
        lineNbField->clear();
    }
}

void MainWindow::updateLoadingProgress( int progress )
{
    LOG( logDEBUG ) << "Loading progress: " << progress;

    QString current_file
        = QDir::toNativeSeparators( session_.getFilename( currentCrawlerWidget() ) );

    // We ignore 0% and 100% to avoid a flash when the file (or update)
    // is very short.
    if ( progress > 0 && progress < 100 ) {
        infoLine->setText( current_file + tr( " - Indexing lines... (%1 %)" ).arg( progress ) );
        infoLine->displayGauge( progress );

        showInfoLabels( false );

        stopAction->setEnabled( true );
        reloadAction->setEnabled( false );
    }
}

void MainWindow::handleLoadingFinished( LoadingStatus status )
{
    LOG( logDEBUG ) << "handleLoadingFinished success=" << ( status == LoadingStatus::Successful );

    // No file is loading
    loadingFileName.clear();

    if ( status == LoadingStatus::Successful ) {
        updateInfoLine();

        infoLine->hideGauge();
        showInfoLabels( true );
        stopAction->setEnabled( false );
        reloadAction->setEnabled( true );

        lineNumberHandler( 0_lnum );

        // Now everything is ready, we can finally show the file!
        currentCrawlerWidget()->show();
    }
    else {
        if ( status == LoadingStatus::NoMemory ) {
            QMessageBox alertBox;
            alertBox.setText( "Not enough memory." );
            alertBox.setInformativeText( "The system does not have enough \
memory to hold the index for this file. The file will now be closed." );
            alertBox.setIcon( QMessageBox::Critical );
            alertBox.exec();
        }

        closeTab( mainTabWidget_.currentIndex() );
    }

    // mainTabWidget_.setEnabled( true );
}

void MainWindow::handleSearchRefreshChanged( bool isRefreshing )
{
    auto& config = Configuration::get();
    config.setSearchAutoRefreshDefault( isRefreshing );
}

void MainWindow::handleMatchCaseChanged( bool matchCase )
{
    auto& config = Configuration::get();
    config.setSearchIgnoreCaseDefault( !matchCase );
}

void MainWindow::closeTab( int index )
{
    auto widget = qobject_cast<CrawlerWidget*>( mainTabWidget_.widget( index ) );

    assert( widget );

    widget->stopLoading();
    mainTabWidget_.removeCrawler( index );
    session_.close( widget );

    updateOpenedFilesMenu();

    widget->deleteLater();
}

void MainWindow::currentTabChanged( int index )
{
    LOG( logDEBUG ) << "currentTabChanged";

    if ( index >= 0 ) {
        auto* crawler_widget = static_cast<CrawlerWidget*>( mainTabWidget_.widget( index ) );
        signalMux_.setCurrentDocument( crawler_widget );
        quickFindMux_.registerSelector( crawler_widget );

        // New tab is set up with fonts etc...
        emit optionsChanged();

        updateMenuBarFromDocument( crawler_widget );
        updateTitleBar( session_.getFilename( crawler_widget ) );
        updateFavoritesMenu();

        editMenu->setEnabled( true );
    }
    else {
        // No tab left
        signalMux_.setCurrentDocument( nullptr );
        quickFindMux_.registerSelector( nullptr );

        infoLine->hideGauge();
        infoLine->clear();
        showInfoLabels( false );

        updateTitleBar( QString() );

        editMenu->setEnabled( false );
        addToFavoritesAction->setEnabled( false );
        addToFavoritesMenuAction->setEnabled( false );
    }
}

void MainWindow::changeQFPattern( const QString& newPattern )
{
    quickFindWidget_.changeDisplayedPattern( newPattern );
}

void MainWindow::loadFileNonInteractive( const QString& file_name )
{
    LOG( logDEBUG ) << "loadFileNonInteractive( " << file_name.toStdString() << " )";

    loadFile( file_name );

    // Try to get the window to the front
    // This is a bit of a hack but has been tested on:
    // Qt 5.3 / Gnome / Linux
    // Qt 5.11 / Win10
#ifdef Q_OS_WIN
    const auto isMaximized = isMaximized_;

    if ( isMaximized ) {
        showMaximized();
    }
    else {
        showNormal();
    }

    activateWindow();
    raise();
#else
    Qt::WindowFlags window_flags = windowFlags();
    window_flags |= Qt::WindowStaysOnTopHint;
    setWindowFlags( window_flags );

    raise();
    activateWindow();

    window_flags = windowFlags();
    window_flags &= ~Qt::WindowStaysOnTopHint;
    setWindowFlags( window_flags );
    showNormal();
#endif

    if ( auto currentCrawler = currentCrawlerWidget() ) {
        currentCrawler->setFocus();
    }
}

//
// Events
//

// Closes the application
void MainWindow::closeEvent( QCloseEvent* event )
{
    if ( !isCloseFromTray_ && this->isVisible() && Configuration::get().minimizeToTray() ) {
        event->ignore();
        trayIcon_->show();
        this->hide();
    }
    else {
        const auto saveSettings = session_.close();
        if ( saveSettings ) {
            writeSettings();
        }

        closeAll();
        trayIcon_->hide();
        emit windowClosed();

        event->accept();
    }
}

// Minimize handling the application
void MainWindow::changeEvent( QEvent* event )
{
    if ( event->type() == QEvent::WindowStateChange ) {
        isMaximized_ = windowState().testFlag( Qt::WindowMaximized );

        if ( this->windowState() & Qt::WindowMinimized ) {
            if ( Configuration::get().minimizeToTray() ) {
                QTimer::singleShot( 0, [this] {
                    trayIcon_->show();
                    this->hide();
                } );
            }
        }
    }
    else if ( event->type() == QEvent::StyleChange ) {
        QTimer::singleShot( 0, [this] {
            loadIcons();
            updateOpenedFilesMenu();
            updateFavoritesMenu();
            updateHighlightersMenu();
        } );
    }

    QMainWindow::changeEvent( event );
}

// Accepts the drag event if it looks like a filename
void MainWindow::dragEnterEvent( QDragEnterEvent* event )
{
    if ( event->mimeData()->hasFormat( "text/uri-list" ) )
        event->acceptProposedAction();
}

// Tries and loads the file if the URL dropped is local
void MainWindow::dropEvent( QDropEvent* event )
{
    QList<QUrl> urls = event->mimeData()->urls();

    for ( const auto& url : qAsConst( urls ) ) {
        auto fileName = url.toLocalFile();
        if ( fileName.isEmpty() )
            continue;

        loadFile( fileName );
    }
}

void MainWindow::keyPressEvent( QKeyEvent* keyEvent )
{
    LOG( logDEBUG4 ) << "keyPressEvent received";

    switch ( keyEvent->key() ) {
    case Qt::Key_Apostrophe:
        displayQuickFindBar( QuickFindMux::Forward );
        break;
    case Qt::Key_QuoteDbl:
        displayQuickFindBar( QuickFindMux::Backward );
        break;
    default:
        keyEvent->ignore();
    }

    if ( ( keyEvent->modifiers().testFlag( Qt::ControlModifier )
           && keyEvent->key() == Qt::Key_S )                        // Ctrl+S
         || ( keyEvent->modifiers().testFlag( Qt::ControlModifier ) // Ctrl+Shift+F
              && keyEvent->modifiers().testFlag( Qt::ShiftModifier )
              && keyEvent->key() == Qt::Key_F ) ) {

        if ( auto crawler = currentCrawlerWidget() ) {
            keyEvent->accept();
            crawler->focusSearchEdit();
        }
    }

    if ( !keyEvent->isAccepted() )
        QMainWindow::keyPressEvent( keyEvent );
}

bool MainWindow::event( QEvent* event )
{
    if ( event->type() == QEvent::WindowActivate ) {
        emit windowActivated();
    }
    else if ( event->type() == QEvent::Show ) {
        if ( this->windowHandle() ) {
            std::call_once( screenChangesConnect_, [this]() {
                logScreenInfo( this->windowHandle()->screen() );
                connect( this->windowHandle(), &QWindow::screenChanged,
                         [this]( QScreen* screen ) { logScreenInfo( screen ); } );
            } );
        }
    }

    return QMainWindow::event( event );
}

//
// Private functions
//

bool MainWindow::extractAndLoadFile( const QString& fileName )
{
    const auto& config = Configuration::get();

    if ( !config.extractArchives() ) {
        return false;
    }

    if ( !config.extractArchivesAlways() ) {
        const auto userChoice
            = QMessageBox::question( this, "klogg", "Extract archive to temp folder?" );
        if ( userChoice == QMessageBox::No ) {
            return false;
        }
    }

    const auto decompressAction = Decompressor::action( fileName );

    Decompressor decompressor;

    QProgressDialog progressDialog;
    progressDialog.setLabelText( QString( "Extracting %1" ).arg( fileName ) );
    progressDialog.setRange( 0, 0 );

    connect( &decompressor, &Decompressor::finished,
             [&progressDialog]( bool isOk ) { progressDialog.done( isOk ? 0 : 1 ); } );

    if ( decompressAction == DecompressAction::Decompress ) {

        auto tempFile = new QTemporaryFile(
            this->tempDir_.filePath( QFileInfo( fileName ).fileName() ), this );
        if ( tempFile->open() && decompressor.decompress( fileName, tempFile )
             && !progressDialog.exec() ) {
            return this->loadFile( tempFile->fileName() );
        }
        else {
            QMessageBox::warning(
                this, "klogg",
                QString( "Failed to decompress %1" ).arg( QDir::toNativeSeparators( fileName ) ) );
        }
    }
    else if ( decompressAction == DecompressAction::Extract ) {
        QTemporaryDir archiveDir{ this->tempDir_.filePath( QFileInfo( fileName ).fileName() ) };
        archiveDir.setAutoRemove( false );
        if ( decompressor.extract( fileName, archiveDir.path() ) && !progressDialog.exec() ) {

            const auto selectedFiles = QFileDialog::getOpenFileNames(
                this, tr( "Open file from archive" ), archiveDir.path(), tr( "All files (*)" ) );

            for ( const auto& extractedFile : selectedFiles ) {
                this->loadFile( extractedFile );
            }

            return true;
        }
        else {
            QMessageBox::warning(
                this, "klogg",
                QString( "Failed to extract %1" ).arg( QDir::toNativeSeparators( fileName ) ) );
        }
    }

    return false;
}

// Create a CrawlerWidget for the passed file, start its loading
// and update the title bar.
// The loading is done asynchronously.
bool MainWindow::loadFile( const QString& fileName, bool followFile )
{
    LOG( logDEBUG ) << "loadFile ( " << fileName.toStdString() << " )";

    // First check if the file is already open...
    auto* existing_crawler = static_cast<CrawlerWidget*>( session_.getViewIfOpen( fileName ) );

    if ( existing_crawler ) {
        auto* crawlerWindow = qobject_cast<MainWindow*>( existing_crawler->window() );
        crawlerWindow->mainTabWidget_.setCurrentWidget( existing_crawler );
        crawlerWindow->activateWindow();
        return true;
    }

    const auto decompressAction = Decompressor::action( fileName );

    if ( decompressAction == DecompressAction::None || !Configuration::get().extractArchives() ) {
        // Load the file
        loadingFileName = fileName;

        try {
            CrawlerWidget* crawler_widget = static_cast<CrawlerWidget*>(
                session_.open( fileName, []() { return new CrawlerWidget(); } ) );

            if ( !crawler_widget ) {
                LOG( logERROR ) << "Can't create crawler for " << fileName.toStdString();
                return false;
            }

            // We won't show the widget until the file is fully loaded
            crawler_widget->hide();

            // We disable the tab widget to avoid having someone switch
            // tab during loading. (maybe FIXME)
            // mainTabWidget_.setEnabled( false );

            int index = mainTabWidget_.addCrawler( crawler_widget, fileName );

            // Setting the new tab, the user will see a blank page for the duration
            // of the loading, with no way to switch to another tab
            mainTabWidget_.setCurrentIndex( index );

            // Update the recent files list
            // (reload the list first in case another glogg changed it)
            auto& recentFiles = RecentFiles::getSynced();
            recentFiles.addRecent( fileName );
            recentFiles.save();
            updateRecentFileActions();

            updateOpenedFilesMenu();

            const auto& config = Configuration::get();
            if ( config.anyFileWatchEnabled() && ( followFile || config.followFileOnLoad() ) ) {
                signalCrawlerToFollowFile( crawler_widget );
                followAction->setChecked( true );
            }
        } catch ( ... ) {
            LOG( logERROR ) << "Can't open file " << fileName.toStdString();
            return false;
        }

        LOG( logDEBUG ) << "Success loading file " << fileName.toStdString();
        return true;
    }
    else {
        return extractAndLoadFile( fileName );
    }
}

// Strips the passed filename from its directory part.
QString MainWindow::strippedName( const QString& fullFileName ) const
{
    return QFileInfo( fullFileName ).fileName();
}

// Return the currently active CrawlerWidget, or NULL if none
CrawlerWidget* MainWindow::currentCrawlerWidget() const
{
    auto current = qobject_cast<CrawlerWidget*>( mainTabWidget_.currentWidget() );

    return current;
}

// Update the title bar.
void MainWindow::updateTitleBar( const QString& file_name )
{
    QString shownName = tr( "Untitled" );
    if ( !file_name.isEmpty() ) {
        shownName = strippedName( file_name );
    }

    QString indexPart = "";
    if ( session_.windowIndex() > 0 ) {
        indexPart = QString( " #%1" ).arg( session_.windowIndex() + 1 );
    }

    setWindowTitle( tr( "%1 - %2%3" ).arg( shownName, tr( "klogg" ), indexPart ) + " (build "
                    + kloggVersion() + ")" );
}

// Updates the actions for the recent files.
// Must be called after having added a new name to the list.
void MainWindow::updateRecentFileActions()
{
    QStringList recent_files = RecentFiles::get().recentFiles();

    for ( auto j = 0; j < MaxRecentFiles; ++j ) {
        const auto actionIndex = static_cast<size_t>( j );
        if ( j < recent_files.count() ) {
            QString text = tr( "&%1 %2" ).arg( j + 1 ).arg( strippedName( recent_files[ j ] ) );
            recentFileActions[ actionIndex ]->setText( text );
            recentFileActions[ actionIndex ]->setToolTip( recent_files[ j ] );
            recentFileActions[ actionIndex ]->setData( recent_files[ j ] );
            recentFileActions[ actionIndex ]->setVisible( true );
        }
        else {
            recentFileActions[ actionIndex ]->setVisible( false );
        }
    }

    // separatorAction->setVisible(!recentFiles.isEmpty());
}

// Update our menu bar to match the settings of the crawler
// (used when the tab is changed)
void MainWindow::updateMenuBarFromDocument( const CrawlerWidget* crawler )
{
    const auto encodingMib = crawler->encodingMib();

    auto encodingActions = encodingGroup->actions();
    auto encodingItem = std::find_if( encodingActions.begin(), encodingActions.end(),
                                      [&encodingMib]( const auto& action ) {
                                          return ( !encodingMib && !action->data().isValid() )
                                                 || ( encodingMib && action->data().isValid()
                                                      && *encodingMib == action->data().toInt() );
                                      } );

    if ( encodingItem != encodingActions.end() ) {
        ( *encodingItem )->setChecked( true );
    }

    bool follow = crawler->isFollowEnabled();
    followAction->setChecked( follow );
}

// Update the top info line from the session
void MainWindow::updateInfoLine()
{
    QLocale defaultLocale;

    // Following should always work as we will only receive enter
    // this slot if there is a crawler connected.
    QString current_file
        = QDir::toNativeSeparators( session_.getFilename( currentCrawlerWidget() ) );

    uint64_t fileSize;
    uint32_t fileNbLine;
    QDateTime lastModified;

    session_.getFileInfo( currentCrawlerWidget(), &fileSize, &fileNbLine, &lastModified );

    infoLine->setText( current_file );
    infoLine->setPath( current_file );
    sizeField->setText( readableSize( fileSize ) );
    encodingField->setText( currentCrawlerWidget()->encodingText() );

    if ( lastModified.isValid() ) {
        const QString date = defaultLocale.toString( lastModified, QLocale::NarrowFormat );
        dateField->setText( tr( "modified on %1" ).arg( date ) );
        dateField->show();
    }
    else {
        dateField->hide();
    }
}

void MainWindow::updateOpenedFilesMenu()
{
    openedFilesMenu->clear();

    const auto& files = session_.openedFiles();

    openedFilesMenu->setEnabled( !files.empty() );

    openedFilesMenu->addAction( selectOpenFileAction );
    openedFilesMenu->addSeparator();

    for ( const auto& file : files ) {
        const auto displayFile = DisplayFilePath{ file };
        auto action = openedFilesMenu->addAction( displayFile.displayName() );

        action->setActionGroup( openedFilesGroup );
        action->setToolTip( displayFile.nativeFullPath() );
        action->setData( displayFile.fullPath() );
    }

    selectOpenFileAction->setDisabled( files.empty() );
}

void MainWindow::updateHighlightersMenu()
{
    highlightersMenu->clear();
    if ( highlightersActionGroup ) {
        highlightersActionGroup->deleteLater();
    }

    highlightersActionGroup = new QActionGroup( this );
    connect( highlightersActionGroup, &QActionGroup::triggered, this,
             &MainWindow::setCurrentHighlighter );

    highlightersMenu->addAction( editHighlightersAction );
    highlightersMenu->addSeparator();

    populateHighlightersMenu( highlightersMenu, highlightersActionGroup );
}

void MainWindow::setCurrentHighlighter( QAction* action )
{
    saveCurrentHighlighterFromAction( action );

    if ( auto current = currentCrawlerWidget() ) {
        current->applyConfiguration();
    }
}

void MainWindow::updateFavoritesMenu()
{
    favoritesMenu->clear();

    favoritesMenu->addAction( addToFavoritesMenuAction );
    favoritesMenu->addAction( removeFromFavoritesAction );

    addToFavoritesMenuAction->setIcon( iconLoader_.load( "icons8-star" ) );

    addToFavoritesAction->setText( tr( "Add to favorites" ) );
    addToFavoritesAction->setIcon( iconLoader_.load( "icons8-star" ) );
    addToFavoritesAction->setData( true );

    const auto& favorites = FavoriteFiles::getSynced().favorites();
    auto crawler = currentCrawlerWidget();

    addToFavoritesAction->setEnabled( crawler != nullptr );
    addToFavoritesMenuAction->setEnabled( crawler != nullptr );
    removeFromFavoritesAction->setEnabled( !favorites.empty() );

    if ( crawler ) {
        const auto path = session_.getFilename( crawler );
        if ( std::any_of( favorites.begin(), favorites.end(), FullPathComparator( path ) ) ) {
            addToFavoritesAction->setText( tr( "Remove from favorites" ) );
            addToFavoritesAction->setIcon( iconLoader_.load( "icons8-star-filled" ) );
            addToFavoritesAction->setData( false );

            addToFavoritesMenuAction->setEnabled( false );
            addToFavoritesMenuAction->setIcon( iconLoader_.load( "icons8-star-filled" ) );
        }
    }

    favoritesMenu->addSeparator();

    for ( const auto& file : favorites ) {
        auto action = favoritesMenu->addAction( file.displayName() );

        action->setActionGroup( favoritesGroup );
        action->setToolTip( file.nativeFullPath() );
        action->setData( file.fullPath() );
    }
}

void MainWindow::addToFavorites()
{
    if ( const auto crawler = currentCrawlerWidget() ) {
        auto& favorites = FavoriteFiles::get();
        const auto path = session_.getFilename( crawler );

        if ( addToFavoritesAction->data().toBool() ) {
            favorites.add( path );
        }
        else {
            favorites.remove( path );
        }

        favorites.save();

        updateFavoritesMenu();
    }
}

void MainWindow::removeFromFavorites()
{
    const auto& favoriteFiles = FavoriteFiles::get();
    const auto& favorites = favoriteFiles.favorites();
    QStringList files;
    std::transform( favorites.begin(), favorites.end(), std::back_inserter( files ),
                    []( const auto& f ) { return f.nativeFullPath(); } );

    auto currentIndex = 0;

    if ( const auto crawler = currentCrawlerWidget() ) {
        const auto currentPath = session_.getFilename( crawler );
        const auto currentItem
            = std::find_if( favorites.begin(), favorites.end(), FullPathComparator( currentPath ) );
        if ( currentItem != favorites.end() ) {
            currentIndex = static_cast<int>( std::distance( favorites.begin(), currentItem ) );
        }
    }

    bool ok = false;
    const auto pathToRemove = QInputDialog::getItem( this, "Remove from favorites",
                                                     "Select item to remove from favorites", files,
                                                     currentIndex, false, &ok );
    if ( ok ) {
        removeFromFavorites( pathToRemove );
    }
}

void MainWindow::removeFromFavorites( const QString& pathToRemove )
{
    auto& favoriteFiles = FavoriteFiles::get();
    const auto& favorites = favoriteFiles.favorites();
    const auto selectedFile = std::find_if( favorites.begin(), favorites.end(),
                                            FullPathNativeComparator( pathToRemove ) );

    if ( selectedFile != favorites.end() ) {
        favoriteFiles.remove( selectedFile->fullPath() );
        favoriteFiles.save();
        updateFavoritesMenu();
    }
}

void MainWindow::removeFromRecent( const QString& pathToRemove )
{
    auto& recentFiles = RecentFiles::get();
    recentFiles.removeRecent( pathToRemove );
    recentFiles.save();
    updateRecentFileActions();
}

void MainWindow::selectOpenedFile()
{
    auto openedFilesPaths = session_.openedFiles();
    std::vector<DisplayFilePath> openedFiles;
    std::transform( openedFilesPaths.begin(), openedFilesPaths.end(),
                    std::back_inserter( openedFiles ),
                    []( const auto& path ) { return DisplayFilePath{ path }; } );

    QStringList filesToShow;
    std::transform( openedFiles.begin(), openedFiles.end(), std::back_inserter( filesToShow ),
                    []( const auto& f ) { return f.nativeFullPath(); } );

    auto selectFileDialog = std::make_unique<QDialog>( this );
    selectFileDialog->setWindowTitle( "klogg -- switch to file" );
    selectFileDialog->setMinimumWidth( 800 );
    selectFileDialog->setMinimumHeight( 600 );

    auto filesModel = std::make_unique<QStringListModel>( filesToShow, selectFileDialog.get() );
    auto filteredModel = std::make_unique<QSortFilterProxyModel>( selectFileDialog.get() );
    filteredModel->setSourceModel( filesModel.get() );

    auto filesView = std::make_unique<QListView>();
    filesView->setModel( filteredModel.get() );
    filesView->setEditTriggers( QAbstractItemView::NoEditTriggers );
    filesView->setSelectionMode( QAbstractItemView::SingleSelection );

    auto filterEdit = std::make_unique<QLineEdit>();
    auto buttonBox
        = std::make_unique<QDialogButtonBox>( QDialogButtonBox::Ok | QDialogButtonBox::Cancel );

    connect( buttonBox.get(), &QDialogButtonBox::accepted, selectFileDialog.get(),
             &QDialog::accept );
    connect( buttonBox.get(), &QDialogButtonBox::rejected, selectFileDialog.get(),
             &QDialog::reject );

    connect( filterEdit.get(), &QLineEdit::textEdited,
             [model = filteredModel.get(), view = filesView.get()]( const QString& filter ) {
                 model->setFilterWildcard( filter );
                 model->invalidate();
                 view->selectionModel()->select( model->index( 0, 0 ),
                                                 QItemSelectionModel::SelectCurrent );
             } );

    QTimer::singleShot( 0, [edit = filterEdit.get()]() { edit->setFocus(); } );

    connect( selectFileDialog.get(), &QDialog::finished,
             [this, openedFiles, dialog = selectFileDialog.get(), model = filteredModel.get(),
              view = filesView.get()]( auto result ) {
                 dialog->deleteLater();
                 if ( result != QDialog::Accepted || !view->selectionModel()->hasSelection() ) {
                     return;
                 }
                 const auto selectedPath
                     = model->data( view->selectionModel()->selectedIndexes().front() ).toString();
                 const auto selectedFile = std::find_if( openedFiles.begin(), openedFiles.end(),
                                                         FullPathNativeComparator( selectedPath ) );

                 if ( selectedFile != openedFiles.end() ) {
                     loadFile( selectedFile->fullPath() );
                 }
             } );

    auto layout = std::make_unique<QVBoxLayout>();
    layout->addWidget( filesView.release() );
    layout->addWidget( filterEdit.release() );
    layout->addWidget( buttonBox.release() );

    selectFileDialog->setLayout( layout.release() );
    selectFileDialog->setModal( true );
    selectFileDialog->open();

    filesModel.release();
    filteredModel.release();
    selectFileDialog.release();
}

void MainWindow::showInfoLabels( bool show )
{
    for ( auto separator : infoToolbarSeparators ) {
        separator->setVisible( show );
    }
    if ( !show ) {
        sizeField->clear();
        dateField->clear();
        encodingField->clear();
        lineNbField->clear();
    }
}

// Write settings to permanent storage
void MainWindow::writeSettings()
{
    // Save the session
    // Generate the ordered list of widgets and their topLine
    std::vector<
        std::tuple<const ViewInterface*, uint64_t, std::shared_ptr<const ViewContextInterface>>>
        widget_list;
    for ( int i = 0; i < mainTabWidget_.count(); ++i ) {
        auto view = qobject_cast<const CrawlerWidget*>( mainTabWidget_.widget( i ) );
        widget_list.emplace_back( view, 0UL, view->context() );
    }
    session_.save( widget_list, saveGeometry() );
}

// Read settings from permanent storage
void MainWindow::readSettings()
{
    // Get and restore the session
    // auto& session = SessionInfo::getSynced();
    /*
     * FIXME: should be in the session
    crawlerWidget->restoreState( session.crawlerState() );
    */

    // History of recent files
    RecentFiles::getSynced();
    updateRecentFileActions();

    FavoriteFiles::getSynced();
    updateFavoritesMenu();

    HighlighterSetCollection::getSynced();
    updateHighlightersMenu();
}

void MainWindow::displayQuickFindBar( QuickFindMux::QFDirection direction )
{
    LOG( logDEBUG ) << "MainWindow::displayQuickFindBar";

    // Warn crawlers so they can save the position of the focus in order
    // to do incremental search in the right view.
    emit enteringQuickFind();

    const auto crawler = currentCrawlerWidget();
    if ( crawler != nullptr && crawler->isPartialSelection() ) {
        auto selection = crawler->getSelectedText();
        if ( !selection.isEmpty() ) {
            quickFindWidget_.changeDisplayedPattern( selection );
        }
    }

    quickFindMux_.setDirection( direction );
    quickFindWidget_.userActivate();
}

void MainWindow::logScreenInfo( QScreen* screen )
{
    LOG( logINFO ) << "screen changed for " << session_.windowIndex();
    if ( screen == nullptr ) {
        return;
    }

    LOG( logINFO ) << "screen name " << screen->name();
    LOG( logINFO ) << "screen size " << screen->size().width() << "x" << screen->size().height();
    LOG( logINFO ) << "screen ratio " << screen->devicePixelRatio();
    LOG( logINFO ) << "screen logical dpi " << screen->logicalDotsPerInch();
    LOG( logINFO ) << "screen physical dpi " << screen->physicalDotsPerInch();
}

void MainWindow::reportIssue() const
{
    const QString version = kloggVersion();
    const QString buildDate = kloggBuildDate();
    const QString commit = kloggCommit();

    const QString os = QSysInfo::prettyProductName();
    const QString kernelType = QSysInfo::kernelType();
    const QString kernelVersion = QSysInfo::kernelVersion();
    const QString arch = QSysInfo::currentCpuArchitecture();
    const QString built_for = QSysInfo::buildAbi();

    const QString body = QString( "Details for the issue\n"
                                  "--------------------\n\n"
                                  "#### What did you do?\n\n\n"
                                  "#### What did you expect to see?\n\n\n"
                                  "#### What did you see instead?\n\n\n"
                                  "Useful extra information\n"
                                  "-------------------------\n"
                                  "> Klogg version %1 (built on %2 from commit %3) [built for %4]\n"
                                  "> running on %5 (%6/%7) [%8]\n"
                                  "> and Qt %9" )
                             .arg( version, buildDate, commit, built_for, os, kernelType,
                                   kernelVersion, arch, qVersion() );

    QUrlQuery query;
    query.addQueryItem( "labels", "type: bug" );
    query.addQueryItem( "body", body );

    QUrl url( "https://github.com/variar/klogg/issues/new" );
    url.setQuery( query );
    QDesktopServices::openUrl( url );
}

void MainWindow::generateDump()
{
    const auto userAction = QMessageBox::warning(
        this, "klogg - generate crash dump",
        QString( "This will shutdown klogg and generate diagnostic crash dump. Continue?" ),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No );

    if ( userAction == QMessageBox::Yes ) {
        throw std::logic_error( "test dump" );
    }
}
