/*
 * Copyright (C) 2014 Nicolas Bonnefon and other contributors
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

#ifndef TABBEDCRAWLERWIDGET_H
#define TABBEDCRAWLERWIDGET_H

#include <QTabBar>
#include <QTabWidget>
#include <qobjectdefs.h>
#include <qtabbar.h>
#include <qwidget.h>

#include "loadingstatus.h"

// This class represents glogg's main widget, a tabbed
// group of CrawlerWidgets.
// This is a very slightly customised QTabWidget, with
// a particular style.

class CrawlerTabBar : public QTabBar {
  Q_OBJECT

  Q_SIGNALS:
    void showTabContextMenu(int tab, QPoint point);

  protected:
    void mouseReleaseEvent( QMouseEvent* ) override;

};

class TabbedCrawlerWidget : public QTabWidget {
    Q_OBJECT
  public:
    TabbedCrawlerWidget();

    template <typename T>
    int addCrawler( T* crawler, const QString& fileName )
    {
        const auto index = QTabWidget::addTab( crawler, QString{} );

        connect( crawler, &T::dataStatusChanged, this, [ this, fileName ]( DataStatus status ) {
            const auto tabsCount = count();
            for ( int i = 0; i < tabsCount; ++i ) {
                if ( tabPathAt( i ) == fileName ) {
                    setTabDataStatus( i, status );
                    return;
                }
            }
        } );

        addTabBarItem( index, fileName );

        return index;
    }

    void removeCrawler( int index );

    // Switch to the next/previous tab, wrapping around at the ends.
    void selectNextTab();
    void selectPreviousTab();

    // Relocate the current tab one position left/right. Does not wrap around.
    void moveCurrentTabLeft();
    void moveCurrentTabRight();

  protected:
    void keyPressEvent( QKeyEvent* event ) override;
    void mouseReleaseEvent( QMouseEvent* event ) override;
    void changeEvent( QEvent* event ) override;

  private:
    void addTabBarItem( int index, const QString& fileName );
    QString tabPathAt( int index ) const;

    // Set the data status (icon) for the tab number 'index'
    void setTabDataStatus( int index, DataStatus status );

    void loadIcons();
    void updateIcon( int index );

  private Q_SLOTS:
    void showContextMenu( int tab, QPoint globalPoint );

  private:
    QIcon olddata_icon_;
    QIcon newdata_icon_;
    QIcon newfiltered_icon_;

    CrawlerTabBar myTabBar_;
};

#endif
