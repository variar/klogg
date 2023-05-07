/*
 * Copyright (C) 2014, 2015 Nicolas Bonnefon and other contributors
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

#include "tabbedcrawlerwidget.h"

#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFileInfo>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>

#include "crawlerwidget.h"

#include "configuration.h"
#include "dispatch_to.h"
#include "iconloader.h"
#include "log.h"
#include "openfilehelper.h"
#include "styles.h"
#include "tabnamemapping.h"

namespace {
constexpr QLatin1String PathKey = QLatin1String( "path", 4 );
constexpr QLatin1String StatusKey = QLatin1String( "status", 6 );
} // namespace

TabbedCrawlerWidget::TabbedCrawlerWidget()
    : QTabWidget()
    , newdata_icon_( ":/images/newdata_icon.png" )
    , newfiltered_icon_( ":/images/newfiltered_icon.png" )
{

    QString tabStyle = "QTabBar::tab { height: 24px; }";
    QString tabCloseButtonStyle = " QTabBar::close-button {\
              height: 12px; width: 12px;\
              subcontrol-origin: padding;\
              subcontrol-position: right;\
              %1}";

    QString backgroundImage;
    QString backgroundHoverImage;

    const auto& config = Configuration::get();
    if ( config.style() == StyleManager::DarkStyleKey ) {
        backgroundImage = ":/images/icons8-close-window-16_inverse.png";
        backgroundHoverImage = ":/images/icons8-close-window-hover-16_inverse.png";
    }

#if defined( Q_OS_MAC )
    // work around Qt MacOSX bug missing tab close icons
    // see: https://bugreports.qt.io/browse/QTBUG-61092
    // still broken in document mode in Qt.5.12.2 !!!!
    if ( config.style() != StyleManager::DarkStyleKey ) {
        backgroundImage
            = ":/qt-project.org/styles/commonstyle/images/standardbutton-closetab-16.png";
        backgroundHoverImage
            = ":/qt-project.org/styles/commonstyle/images/standardbutton-closetab-hover-16.png";
    }
#elif defined( Q_OS_WIN )
    if ( config.style() == StyleManager::FusionKey ) {
        backgroundImage = ":/images/icons8-close-window-16.png";
        backgroundHoverImage = ":/images/icons8-close-window-hover-16.png";
    }
#endif

    if ( !backgroundImage.isEmpty() ) {
        const QString backgroundImageTemplate = " image: url(%1);";
        QString tabCloseButtonHoverStyle = " QTabBar::close-button:hover { %1 }";
        backgroundImage = backgroundImageTemplate.arg( backgroundImage );
        backgroundHoverImage = backgroundImageTemplate.arg( backgroundHoverImage );
        tabCloseButtonHoverStyle = tabCloseButtonHoverStyle.arg( backgroundHoverImage );
        tabCloseButtonStyle = tabCloseButtonStyle.arg( backgroundImage );
        tabCloseButtonStyle.append( tabCloseButtonHoverStyle );
    }
    else {
        tabCloseButtonStyle = tabCloseButtonStyle.arg( "" );
    }

    myTabBar_.setStyleSheet( tabStyle.append( tabCloseButtonStyle ) );

    setTabBar( &myTabBar_ );
    myTabBar_.hide();

    myTabBar_.setContextMenuPolicy( Qt::CustomContextMenu );
    connect( &myTabBar_, &QWidget::customContextMenuRequested, this,
             &TabbedCrawlerWidget::showContextMenu );

    dispatchToMainThread( [ this ] { loadIcons(); } );
}

void TabbedCrawlerWidget::loadIcons()
{
    IconLoader iconLoader{ this };
    olddata_icon_ = iconLoader.load( "olddata_icon" );
    for ( int tab = 0; tab < count(); ++tab ) {
        updateIcon( tab );
    }
}

void TabbedCrawlerWidget::changeEvent( QEvent* event )
{
    if ( event->type() == QEvent::StyleChange ) {
        dispatchToMainThread( [ this ] { loadIcons(); } );
    }

    QWidget::changeEvent( event );
}

void TabbedCrawlerWidget::addTabBarItem( int index, const QString& fileName )
{
    const auto tabLabel = QFileInfo( fileName ).fileName();
    const auto tabName = TabNameMapping::getSynced().tabName( fileName );

    myTabBar_.setTabIcon( index, olddata_icon_ );
    myTabBar_.setTabText( index, tabName.isEmpty() ? tabLabel : tabName );
    myTabBar_.setTabToolTip( index, QDir::toNativeSeparators( fileName ) );

    QVariantMap tabData;
    tabData[ PathKey ] = fileName;
    tabData[ StatusKey ] = static_cast<int>( DataStatus::OLD_DATA );

    myTabBar_.setTabData( index, tabData );

    if ( count() > 1 )
        myTabBar_.show();
}

void TabbedCrawlerWidget::removeCrawler( int index )
{
    QTabWidget::removeTab( index );

    if ( count() <= 1 )
        myTabBar_.hide();
}

void TabbedCrawlerWidget::mouseReleaseEvent( QMouseEvent* event )
{
    LOG_DEBUG << "TabbedCrawlerWidget::mouseReleaseEvent";

    if ( event->button() == Qt::MiddleButton ) {
        int tab = this->myTabBar_.tabAt( event->pos() );
        if ( -1 != tab ) {
            Q_EMIT tabCloseRequested( tab );
            event->accept();
        }
    }

    event->ignore();
}

QString TabbedCrawlerWidget::tabPathAt( int index ) const
{
    return myTabBar_.tabData( index ).toMap()[ PathKey ].toString();
}

void TabbedCrawlerWidget::showContextMenu( const QPoint& point )
{
    int tab = myTabBar_.tabAt( point );
    if ( -1 != tab ) {
        QMenu menu( this );
        auto closeThis = menu.addAction( tr( "Close this" ) );
        auto closeOthers = menu.addAction( tr( "Close others" ) );
        auto closeLeft = menu.addAction( tr( "Close to the left" ) );
        auto closeRight = menu.addAction( tr( "Close to the right" ) );
        auto closeAll = menu.addAction( tr( "Close all" ) );
        menu.addSeparator();
        auto copyFullPath = menu.addAction( tr( "Copy full path" ) );
        auto openContainingFolder = menu.addAction( tr( "Open containing folder" ) );
        menu.addSeparator();
        auto renameTab = menu.addAction( tr( "Rename tab" ) );
        auto resetTabName = menu.addAction( tr( "Reset tab name" ) );

        connect( closeThis, &QAction::triggered,
                 [ tab, this ] { Q_EMIT tabCloseRequested( tab ); } );

        connect( closeOthers, &QAction::triggered, [ tabWidget = widget( tab ), this ] {
            while ( count() != 1 ) {
                for ( int i = 0; i < count(); ++i ) {
                    if ( i != indexOf( tabWidget ) ) {
                        Q_EMIT tabCloseRequested( i );
                        break;
                    }
                }
            }
        } );

        connect( closeLeft, &QAction::triggered, [ tabWidget = widget( tab ), this ] {
            while ( indexOf( tabWidget ) != 0 ) {
                Q_EMIT tabCloseRequested( 0 );
            }
        } );

        connect( closeRight, &QAction::triggered, [ tab, this ] {
            while ( count() > tab + 1 ) {
                Q_EMIT tabCloseRequested( tab + 1 );
            }
        } );

        connect( closeAll, &QAction::triggered, [ this ] {
            while ( count() ) {
                Q_EMIT tabCloseRequested( 0 );
            }
        } );

        if ( tab == 0 ) {
            closeLeft->setDisabled( true );
        }
        else if ( tab == count() - 1 ) {
            closeRight->setDisabled( true );
        }

        connect( copyFullPath, &QAction::triggered,
                 [ this, tab ] { QApplication::clipboard()->setText( tabToolTip( tab ) ); } );

        connect( openContainingFolder, &QAction::triggered,
                 [ this, tab ] { showPathInFileExplorer( tabToolTip( tab ) ); } );

        connect( renameTab, &QAction::triggered, [ this, tab ] {
            bool isNameEntered = false;
            auto newName = QInputDialog::getText( this, "Rename tab", "Tab name", QLineEdit::Normal,
                                                  myTabBar_.tabText( tab ), &isNameEntered );
            if ( isNameEntered ) {
                const auto tabPath = tabPathAt( tab );
                TabNameMapping::getSynced().setTabName( tabPath, newName ).save();

                if ( newName.isEmpty() ) {
                    myTabBar_.setTabText( tab, QFileInfo( tabPath ).fileName() );
                }
                else {
                    myTabBar_.setTabText( tab, std::move( newName ) );
                }
            }
        } );

        connect( resetTabName, &QAction::triggered, [ this, tab ] {
            const auto tabPath = tabPathAt( tab );
            TabNameMapping::getSynced().setTabName( tabPath, "" ).save();
            myTabBar_.setTabText( tab, QFileInfo( tabPath ).fileName() );
        } );

        menu.exec( myTabBar_.mapToGlobal( point ) );
    }
}

void TabbedCrawlerWidget::keyPressEvent( QKeyEvent* event )
{
    const auto mod = event->modifiers();
    const auto key = event->key();

    LOG_DEBUG << "TabbedCrawlerWidget::keyPressEvent";

    // Ctrl + tab
    if ( ( mod == Qt::ControlModifier && key == Qt::Key_Tab )
         || ( mod == Qt::ControlModifier && key == Qt::Key_PageDown )
         || ( mod == ( Qt::ControlModifier | Qt::AltModifier | Qt::KeypadModifier )
              && key == Qt::Key_Right ) ) {
        setCurrentIndex( ( currentIndex() + 1 ) % count() );
    }
    // Ctrl + shift + tab
    else if ( ( mod == ( Qt::ControlModifier | Qt::ShiftModifier ) && key == Qt::Key_Tab )
              || ( mod == Qt::ControlModifier && key == Qt::Key_PageUp )
              || ( mod == ( Qt::ControlModifier | Qt::AltModifier | Qt::KeypadModifier )
                   && key == Qt::Key_Left ) ) {
        setCurrentIndex( ( currentIndex() - 1 >= 0 ) ? currentIndex() - 1 : count() - 1 );
    }
    // Ctrl + numbers
    else if ( mod == Qt::ControlModifier && ( key >= Qt::Key_1 && key <= Qt::Key_8 ) ) {
        int newIndex = key - Qt::Key_0;
        if ( newIndex <= count() )
            setCurrentIndex( newIndex - 1 );
    }
    // Ctrl + 9
    else if ( mod == Qt::ControlModifier && key == Qt::Key_9 ) {
        setCurrentIndex( count() - 1 );
    }
    else if ( mod == Qt::ControlModifier && ( key == Qt::Key_Q || key == Qt::Key_W ) ) {
        Q_EMIT tabCloseRequested( currentIndex() );
    }
    else {
        QTabWidget::keyPressEvent( event );
    }
}

void TabbedCrawlerWidget::updateIcon( int index )
{
    auto tabData = myTabBar_.tabData( index ).toMap();

    const QIcon* icon;
    switch ( static_cast<DataStatus>( tabData[ StatusKey ].toInt() ) ) {
    case DataStatus::OLD_DATA:
        icon = &olddata_icon_;
        break;
    case DataStatus::NEW_DATA:
        icon = &newdata_icon_;
        break;
    case DataStatus::NEW_FILTERED_DATA:
        icon = &newfiltered_icon_;
        break;
    default:
        return;
    }

    myTabBar_.setTabIcon( index, *icon );
}

void TabbedCrawlerWidget::setTabDataStatus( int index, DataStatus status )
{
    LOG_DEBUG << "TabbedCrawlerWidget::setTabDataStatus " << index;

    auto tabData = myTabBar_.tabData( index ).toMap();
    tabData[ StatusKey ] = static_cast<int>( status );
    myTabBar_.setTabData( index, tabData );

    updateIcon( index );
}
