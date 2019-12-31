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
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QProgressDialog>
#include <QTemporaryFile>
#include <QToolBar>
#include <QUrl>
#include <QWindow>

#include "downloader.h"
#include "log.h"
#include "openfilehelper.h"

#include "mainwindow.h"

#include "crawlerwidget.h"
#include "encodings.h"
#include "favoritefiles.h"
#include "highlightersdialog.h"
#include "optionsdialog.h"
#include "recentfiles.h"
#include "tabbedcrawlerwidget.h"

#include "version.h"

// Returns the size in human readable format
static QString readableSize( qint64 size );

namespace {

void signalCrawlerToFollowFile( CrawlerWidget* crawler_widget )
{
    QMetaObject::invokeMethod( crawler_widget, "followSet", Q_ARG( bool, true ) );
}

} // namespace

MainWindow::MainWindow( WindowSession session )
    : session_( std::move( session ) )
    , mainIcon_()
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

#ifdef GLOGG_SUPPORTS_VERSION_CHECKING

#endif

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
}

void MainWindow::reloadGeometry()
{
    QByteArray geometry;

    session_.restoreGeometry( &geometry );
    restoreGeometry( geometry );
}

void MainWindow::reloadSession()
{
    const auto followFileOnLoad = Configuration::get().followFileOnLoad();

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
    openAction->setIcon( QIcon( ":/images/icons8-add-file-16.png" ) );
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
    connect( recentFilesGroup, &QActionGroup::triggered, this, &MainWindow::openFileFromAction );
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
    followAction->setIcon( QIcon( ":/images/icons8-refresh-16.png" ) );

    followAction->setShortcuts( QList<QKeySequence>()
                                << QKeySequence( Qt::Key_F ) << QKeySequence( Qt::Key_F10 ) );

    followAction->setCheckable( true );
    connect( followAction, &QAction::toggled, this, &MainWindow::followSet );

    reloadAction = new QAction( tr( "&Reload" ), this );
    reloadAction->setShortcuts( QKeySequence::keyBindings( QKeySequence::Refresh ) );
    reloadAction->setIcon( QIcon( ":/images/icons8-restore-page-16.png" ) );
    signalMux_.connect( reloadAction, SIGNAL( triggered() ), SLOT( reload() ) );

    stopAction = new QAction( tr( "&Stop" ), this );
    stopAction->setIcon( QIcon( ":/images/icons8-delete-16.png" ) );
    stopAction->setEnabled( true );
    signalMux_.connect( stopAction, SIGNAL( triggered() ), SLOT( stopLoading() ) );

    highlightersAction = new QAction( tr( "&Highlighters..." ), this );
    highlightersAction->setStatusTip( tr( "Show the Highlighters box" ) );
    connect( highlightersAction, &QAction::triggered, [this]( auto ) { this->highlighters(); } );

    optionsAction = new QAction( tr( "&Options..." ), this );
    optionsAction->setStatusTip( tr( "Show the Options box" ) );
    connect( optionsAction, &QAction::triggered, [this]( auto ) { this->options(); } );

    aboutAction = new QAction( tr( "&About" ), this );
    aboutAction->setStatusTip( tr( "Show the About box" ) );
    connect( aboutAction, &QAction::triggered, [this]( auto ) { this->about(); } );

    aboutQtAction = new QAction( tr( "About &Qt" ), this );
    aboutQtAction->setStatusTip( tr( "Show the Qt library's About box" ) );
    connect( aboutQtAction, &QAction::triggered, [this]( auto ) { this->aboutQt(); } );

    showScratchPadAction = new QAction( tr( "Scratchpad" ), this );
    showScratchPadAction->setStatusTip( tr( "Show the scratchpad" ) );
    showScratchPadAction->setIcon( QIcon( ":/images/icons8-create-16.png" ) );
    connect( showScratchPadAction, &QAction::triggered,
             [this]( auto ) { this->showScratchPad(); } );

    encodingGroup = new QActionGroup( this );
    connect( encodingGroup, &QActionGroup::triggered, this, &MainWindow::encodingChanged );

    favoritesGroup = new QActionGroup( this );
    connect( favoritesGroup, &QActionGroup::triggered, this, &MainWindow::openFileFromAction );

    addToFavoritesAction = new QAction( tr( "Add to favorites" ), this );
    addToFavoritesAction->setIcon( QIcon( ":/images/icons8-star-16.png" ) );
    addToFavoritesAction->setData( true );
    connect( addToFavoritesAction, &QAction::triggered,
             [this]( auto ) { this->addToFavorites(); } );

    addToFavoritesMenuAction = new QAction( tr( "Add to favorites" ), this );
    addToFavoritesMenuAction->setIcon( QIcon( ":/images/icons8-star-16.png" ) );
    connect( addToFavoritesMenuAction, &QAction::triggered,
             [this]( auto ) { this->addToFavorites(); } );

    removeFromFavoritesAction = new QAction( tr( "Remove from favorites..." ), this );
    connect( removeFromFavoritesAction, &QAction::triggered,
             [this]( auto ) { this->removeFromFavorites(); } );
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
    viewMenu->addAction( overviewVisibleAction );
    viewMenu->addSeparator();
    viewMenu->addAction( lineNumbersVisibleInMainAction );
    viewMenu->addAction( lineNumbersVisibleInFilteredAction );
    viewMenu->addSeparator();
    viewMenu->addAction( followAction );
    viewMenu->addSeparator();
    viewMenu->addAction( reloadAction );

    toolsMenu = menuBar()->addMenu( tr( "&Tools" ) );
    toolsMenu->addAction( highlightersAction );
    toolsMenu->addSeparator();
    toolsMenu->addAction( optionsAction );

    toolsMenu->addSeparator();
    toolsMenu->addAction( showScratchPadAction );

    menuBar()->addMenu( EncodingMenu::generate( encodingGroup ) );
    menuBar()->addSeparator();

    favoritesMenu = menuBar()->addMenu( tr( "Favorites" ) );
    favoritesMenu->setToolTipsVisible( true );

    helpMenu = menuBar()->addMenu( tr( "&Help" ) );
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
                 progressDialog.setRange( 0, bytesTotal );
                 progressDialog.setValue( bytesReceived );
             } );

    connect( &downloader, &Downloader::finished,
             [&progressDialog]( bool isOk ) { progressDialog.done( isOk ? 0 : 1 ); } );

    auto tempFile
        = new QTemporaryFile( tempDir_.filePath( url.fileName() ), this );
    if ( tempFile->open() ) {
        downloader.download( url, tempFile );
        if ( !progressDialog.exec() ) {
            loadFile( tempFile->fileName() );
        }
    }
}

// Opens a log file from the recent files list
void MainWindow::openFileFromAction( QAction* action )
{
    if ( action )
        loadFile( action->data().toString() );
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
    CrawlerWidget* current = currentCrawlerWidget();

    if ( current ) {
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
    auto text = clipboard->text();
    openClipboardAction->setEnabled( !text.isEmpty() );
}

void MainWindow::openClipboard()
{
    auto clipboard = QGuiApplication::clipboard();
    auto text = clipboard->text();
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
    QString url = QInputDialog::getText( this, tr( "Open URL as log file" ), tr( "URL:" ),
                                         QLineEdit::Normal, "", &ok );
    if ( ok && !url.isEmpty() ) {
        openRemoteFile( url );
    }
}

// Opens the 'Highlighters' dialog box
void MainWindow::highlighters()
{
    HighlightersDialog dialog( this );
    signalMux_.connect( &dialog, SIGNAL( optionsChanged() ), SLOT( applyConfiguration() ) );
    dialog.exec();
    signalMux_.disconnect( &dialog, SIGNAL( optionsChanged() ), SLOT( applyConfiguration() ) );
}

// Opens the 'Options' modal dialog box
void MainWindow::options()
{
    OptionsDialog dialog( this );
    signalMux_.connect( &dialog, SIGNAL( optionsChanged() ), SLOT( applyConfiguration() ) );
    dialog.exec();
    signalMux_.disconnect( &dialog, SIGNAL( optionsChanged() ), SLOT( applyConfiguration() ) );

    const auto& config = Configuration::get();
    plog::EnableLogging( config.enableLogging(), config.loggingLevel() );

    newWindowAction->setVisible( config.allowMultipleWindows() );
}

// Opens the 'About' dialog box.
void MainWindow::about()
{
    QMessageBox::about(
        this, tr( "About klogg" ),
        tr( "<h2>klogg " GLOGG_VERSION "</h2>"
            "<p>A fast, advanced log explorer.</p>"
#ifdef GLOGG_COMMIT
            "<p>Built " GLOGG_DATE " from " GLOGG_COMMIT "</p>"
#endif
            "<p><a href=\"https://github.com/variar/klogg\">https://github.com/variar/klogg</a></p>"
            "<p>This is fork of glogg</p>"
            "<p><a href=\"http://glogg.bonnefon.org/\">http://glogg.bonnefon.org/</a></p>"
            "<p>Using icons form <a href=\"https://icons8.com\">icons8.com</a> project</p>"
            "<p>Copyright &copy; 2019 Nicolas Bonnefon, Anton Filimonov and other contributors</p>"
            "<p>You may modify and redistribute the program under the terms of the GPL (version 3 "
            "or later).</p>" ) );
}

// Opens the 'About Qt' dialog box.
void MainWindow::aboutQt()
{
    QMessageBox::aboutQt( this, tr( "About Qt" ) );
}

void MainWindow::showScratchPad()
{
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
    currentCrawlerWidget()->setEncoding( mib );
    updateInfoLine();
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

        if ( getArhiveType( fileName ) == Archive::None ) {
            loadFile( fileName );
        }
        else {
            QTemporaryDir archiveDir{tempDir_.filePath( QFileInfo(fileName).fileName() )};
            archiveDir.setAutoRemove(false);

            if (extractArchive(fileName, archiveDir.path())) {
                const auto selectedFiles = QFileDialog::getOpenFileNames(
                    this, tr( "Open file from archive" ), archiveDir.path(), tr( "All files (*)" ) );

                for ( const auto& extractedFile : selectedFiles ) {
                    loadFile( extractedFile );
                }
            }

        }
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

    if ( !keyEvent->isAccepted() )
        QMainWindow::keyPressEvent( keyEvent );
}

bool MainWindow::event( QEvent* event )
{
    if ( event->type() == QEvent::WindowActivate ) {
        emit windowActivated();
    }

    return QMainWindow::event( event );
}

//
// Private functions
//

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

        const auto& config = Configuration::get();
        if ( followFile || config.followFileOnLoad() ) {
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

    setWindowTitle( tr( "%1 - %2%3" ).arg( shownName, tr( "klogg" ), indexPart )
#ifdef GLOGG_COMMIT
                    + " (build " GLOGG_VERSION ")"
#endif
    );
}

// Updates the actions for the recent files.
// Must be called after having added a new name to the list.
void MainWindow::updateRecentFileActions()
{
    QStringList recent_files = RecentFiles::get().recentFiles();

    for ( int j = 0; j < MaxRecentFiles; ++j ) {
        if ( j < recent_files.count() ) {
            QString text = tr( "&%1 %2" ).arg( j + 1 ).arg( strippedName( recent_files[ j ] ) );
            recentFileActions[ j ]->setText( text );
            recentFileActions[ j ]->setToolTip( recent_files[ j ] );
            recentFileActions[ j ]->setData( recent_files[ j ] );
            recentFileActions[ j ]->setVisible( true );
        }
        else {
            recentFileActions[ j ]->setVisible( false );
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

void MainWindow::updateFavoritesMenu()
{
    favoritesMenu->clear();

    favoritesMenu->addAction( addToFavoritesMenuAction );
    favoritesMenu->addAction( removeFromFavoritesAction );

    addToFavoritesMenuAction->setIcon( QIcon( ":/images/icons8-star-16.png" ) );

    addToFavoritesAction->setText( tr( "Add to favorites" ) );
    addToFavoritesAction->setIcon( QIcon( ":/images/icons8-star-16.png" ) );
    addToFavoritesAction->setData( true );

    const auto& favorites = FavoriteFiles::getSynced().favorites();
    auto crawler = currentCrawlerWidget();

    addToFavoritesAction->setEnabled( crawler != nullptr );
    addToFavoritesMenuAction->setEnabled( crawler != nullptr );
    removeFromFavoritesAction->setEnabled( !favorites.empty() );

    if ( crawler ) {
        const auto path = session_.getFilename( crawler );
        if ( std::any_of( favorites.begin(), favorites.end(),
                          FavoriteFiles::FullPathComparator( path ) ) ) {
            addToFavoritesAction->setText( tr( "Remove from favorites" ) );
            addToFavoritesAction->setIcon( QIcon( ":/images/icons8-star-filled-16.png" ) );
            addToFavoritesAction->setData( false );

            addToFavoritesMenuAction->setEnabled( false );
            addToFavoritesMenuAction->setIcon( QIcon( ":/images/icons8-star-filled-16.png" ) );
        }
    }

    favoritesMenu->addSeparator();

    for ( const auto& file : favorites ) {
        auto action = favoritesMenu->addAction( file.displayName );

        action->setActionGroup( favoritesGroup );
        action->setToolTip( file.fullPathNative );
        action->setData( file.fullPath );
    }
}

void MainWindow::addToFavorites()
{
    auto favorites = FavoriteFiles::get();
    const auto path = session_.getFilename( currentCrawlerWidget() );

    if ( addToFavoritesAction->data().toBool() ) {
        favorites.add( path );
    }
    else {
        favorites.remove( path );
    }

    favorites.save();

    updateFavoritesMenu();
}

void MainWindow::removeFromFavorites()
{
    auto favoriteFiles = FavoriteFiles::get();
    const auto& favorites = favoriteFiles.favorites();
    QStringList files;
    std::transform( favorites.begin(), favorites.end(), std::back_inserter( files ),
                    []( const auto& f ) { return f.fullPathNative; } );

    const auto currentPath = session_.getFilename( currentCrawlerWidget() );
    const auto currentItem = std::find_if( favorites.begin(), favorites.end(),
                                           FavoriteFiles::FullPathComparator( currentPath ) );
    auto currentIndex = 0;
    if ( currentItem != favorites.end() ) {
        currentIndex = std::distance( favorites.begin(), currentItem );
    }

    bool ok = false;
    const auto pathToRemove = QInputDialog::getItem( this, "Remove from favorites",
                                                     "Select item to remove from favorites", files,
                                                     currentIndex, false, &ok );
    if ( ok ) {
        const auto selectedFile
            = std::find_if( favorites.begin(), favorites.end(),
                            FavoriteFiles::FullPathNativeComparator( pathToRemove ) );

        if ( selectedFile != favorites.end() ) {
            favoriteFiles.remove( selectedFile->fullPath );
            favoriteFiles.save();
            updateFavoritesMenu();
        }
    }
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

    HighlighterSet::getSynced();

    FavoriteFiles::getSynced();
    updateFavoritesMenu();
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

// Returns the size in human readable format
static QString readableSize( qint64 size )
{
    static const QString sizeStrs[]
        = { QObject::tr( "B" ), QObject::tr( "KiB" ), QObject::tr( "MiB" ), QObject::tr( "GiB" ),
            QObject::tr( "TiB" ) };

    QLocale defaultLocale;
    unsigned int i;
    double humanSize = size;

    for ( i = 0;
          i + 1 < ( sizeof( sizeStrs ) / sizeof( QString ) ) && ( humanSize / 1024.0 ) >= 1024.0;
          i++ )
        humanSize /= 1024.0;

    if ( humanSize >= 1024.0 ) {
        humanSize /= 1024.0;
        i++;
    }

    QString output;
    if ( i == 0 )
        // No decimal part if we display straight bytes.
        output = defaultLocale.toString( static_cast<int>( humanSize ) );
    else
        output = defaultLocale.toString( humanSize, 'f', 1 );

    output += QString( " " ) + sizeStrs[ i ];

    return output;
}
