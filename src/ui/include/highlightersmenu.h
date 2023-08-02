/*
 * Copyright (C) 2020 Anton Filimonov and other contributors
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

#ifndef HIGHLIGHTERSMENU_H
#define HIGHLIGHTERSMENU_H

#include "highlighterset.h"

#include <functional>

#include <QAction>
#include <QActionGroup>
#include <QMenu>

#include "menu.h"

class CrawlerWidget;

class HighlightersMenu : public HoverMenu {
    Q_OBJECT

  public:
    using HoverMenu::addAction;

    HighlightersMenu( const QString& title, QWidget* parent = nullptr );

    void addAction( QAction* action, bool seq );

    void clearHighlightersMenu();

    void createHighlightersMenu();

    void populateHighlightersMenu();

    inline void setApplyChange( std::function<void()> apply )
    {
        applyChange_ = apply;
    }

  private:
    using HoverMenu::clear;

    // save highlighter action
    void saveCurrentHighlighterFromAction( const QAction* action ) const;

  private Q_SLOTS:
    void applySelectionHighlighters( QAction* action ) const;

    void updateActionsStatus() const;

  private:
    QActionGroup* highLighters_;
    std::function<void()> applyChange_;
};

#endif
