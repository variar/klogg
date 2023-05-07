/*
 * Copyright (C) 2021 Anton Filimonov and other contributors
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

#include <algorithm>
#include <functional>

#include <QApplication>
#include <QShortcut>
#include <QWidget>

#include "shortcuts.h"

const std::map<std::string, QStringList>& ShortcutAction::defaultShortcuts()
{
    static const std::map<std::string, QStringList> defaultShortcuts = []() {
        std::map<std::string, QStringList> shortcuts;

        auto getKeyBindings = []( QKeySequence::StandardKey standardKey ) {
            auto bindings = QKeySequence::keyBindings( standardKey );
            QStringList stringBindings;
            std::transform( bindings.cbegin(), bindings.cend(),
                            std::back_inserter( stringBindings ),
                            []( const auto& keySequence ) { return keySequence.toString(); } );

            return stringBindings;
        };

        shortcuts.emplace( MainWindowNewWindow, QStringList() );
        shortcuts.emplace( MainWindowOpenFile, getKeyBindings( QKeySequence::Open ) );
        shortcuts.emplace( MainWindowCloseFile, getKeyBindings( QKeySequence::Close ) );
        shortcuts.emplace( MainWindowCloseAll, QStringList() );
        shortcuts.emplace( MainWindowQuit, QStringList() << "Ctrl+Q" );
        shortcuts.emplace( MainWindowCopy, getKeyBindings( QKeySequence::Copy ) );
        shortcuts.emplace( MainWindowSelectAll, QStringList() << "Ctrl+A" );
        shortcuts.emplace( MainWindowFocusSearchInput, QStringList() << "Ctrl+S"
                                                                     << "Ctrl+Shift+F" );
        shortcuts.emplace( MainWindowOpenQf, getKeyBindings( QKeySequence::Find ) );
        shortcuts.emplace( MainWindowOpenQfForward,
                           QStringList() << QKeySequence( Qt::Key_Apostrophe ).toString() );
        shortcuts.emplace( MainWindowOpenQfBackward,
                           QStringList() << QKeySequence( Qt::Key_QuoteDbl ).toString() );

        shortcuts.emplace( MainWindowClearFile, getKeyBindings( QKeySequence::Cut ) );
        shortcuts.emplace( MainWindowOpenContainingFolder, QStringList() );
        shortcuts.emplace( MainWindowOpenInEditor, QStringList() );
        shortcuts.emplace( MainWindowCopyPathToClipboard, QStringList() );
        shortcuts.emplace( MainWindowOpenFromClipboard, getKeyBindings( QKeySequence::Paste ) );
        shortcuts.emplace( MainWindowOpenFromUrl, QStringList() );
        shortcuts.emplace( MainWindowFollowFile, QStringList()
                                                     << QKeySequence( Qt::Key_F ).toString()
                                                     << QKeySequence( Qt::Key_F10 ).toString() );
        shortcuts.emplace( MainWindowReload, getKeyBindings( QKeySequence::Refresh ) );
        shortcuts.emplace( MainWindowStop, getKeyBindings( QKeySequence::Cancel ) );
        shortcuts.emplace( MainWindowScratchpad, QStringList() );
        shortcuts.emplace( MainWindowSelectOpenFile, QStringList() << "Ctrl+Shift+O" );

        shortcuts.emplace( CrawlerChangeVisibility, QStringList()
                                                        << QKeySequence( Qt::Key_V ).toString() );
        shortcuts.emplace( CrawlerIncreseTopViewSize,
                           QStringList() << QKeySequence( Qt::Key_Plus ).toString() );
        shortcuts.emplace( CrawlerDecreaseTopViewSize,
                           QStringList() << QKeySequence( Qt::Key_Minus ).toString() );

        // shortcuts.emplace( QfFindNext, getKeyBindings( QKeySequence::FindNext ) );
        // shortcuts.emplace( QfFindPrev, getKeyBindings( QKeySequence::FindPrevious ) );

        shortcuts.emplace( LogViewMark, QStringList() << QKeySequence( Qt::Key_M ).toString() );

        shortcuts.emplace( LogViewNextMark,
                           QStringList() << QKeySequence( Qt::Key_BracketRight ).toString() );
        shortcuts.emplace( LogViewPrevMark, QStringList()
                                                << QKeySequence( Qt::Key_BracketLeft ).toString() );
        shortcuts.emplace( LogViewSelectionUp, QStringList()
                                                   << QKeySequence( Qt::Key_Up ).toString()
                                                   << QKeySequence( Qt::Key_K ).toString() );
        shortcuts.emplace( LogViewSelectionDown, QStringList()
                                                     << QKeySequence( Qt::Key_Down ).toString()
                                                     << QKeySequence( Qt::Key_J ).toString() );
        shortcuts.emplace( LogViewScrollUp, QStringList() << "Ctrl+Up" );
        shortcuts.emplace( LogViewScrollDown, QStringList() << "Ctrl+Down" );
        shortcuts.emplace( LogViewScrollLeft, QStringList()
                                                  << QKeySequence( Qt::Key_Left ).toString()
                                                  << QKeySequence( Qt::Key_H ).toString() );
        shortcuts.emplace( LogViewScrollRight, QStringList()
                                                   << QKeySequence( Qt::Key_Right ).toString()
                                                   << QKeySequence( Qt::Key_L ).toString() );
        shortcuts.emplace( LogViewJumpToStartOfLine,
                           QStringList() << QKeySequence( Qt::Key_Home ).toString()
                                         << QKeySequence( Qt::Key_AsciiCircum ).toString() );
        shortcuts.emplace( LogViewJumpToEndOfLine,
                           QStringList() << QKeySequence( Qt::Key_Dollar ).toString() );
        shortcuts.emplace( LogViewJumpToRightOfScreen,
                           QStringList() << QKeySequence( Qt::Key_End ).toString() );
        shortcuts.emplace( LogViewJumpToButtom, QStringList() << "Ctrl+End"
                                                              << "Shift+G" );
        shortcuts.emplace( LogViewJumpToTop, QStringList() << "Ctrl+Home" );
        shortcuts.emplace( LogViewJumpToLine, QStringList() << "Ctrl+L" );
        shortcuts.emplace( LogViewQfForward, getKeyBindings( QKeySequence::FindNext )
                                                 << QKeySequence( Qt::Key_N ).toString()
                                                 << "Ctrl+G" );
        shortcuts.emplace( LogViewQfBackward, getKeyBindings( QKeySequence::FindPrevious )
                                                  << "Shift+N"
                                                  << "Ctrl+Shift+G" );

        shortcuts.emplace( LogViewQfSelectedForward,
                           QStringList() << QKeySequence( Qt::Key_Asterisk ).toString()
                                         << QKeySequence( Qt::Key_Period ).toString() );
        shortcuts.emplace( LogViewQfSelectedBackward,
                           QStringList() << QKeySequence( Qt::Key_Slash ).toString()
                                         << QKeySequence( Qt::Key_Comma ).toString() );
        shortcuts.emplace( LogViewExitView, QStringList()
                                                << QKeySequence( Qt::Key_Space ).toString() );

        shortcuts.emplace( LogViewAddColorLabel1, QStringList() << "Ctrl+Shift+1" );
        shortcuts.emplace( LogViewAddColorLabel2, QStringList() << "Ctrl+Shift+2" );
        shortcuts.emplace( LogViewAddColorLabel3, QStringList() << "Ctrl+Shift+3" );
        shortcuts.emplace( LogViewAddColorLabel4, QStringList() << "Ctrl+Shift+4" );
        shortcuts.emplace( LogViewAddColorLabel5, QStringList() << "Ctrl+Shift+5" );
        shortcuts.emplace( LogViewAddColorLabel6, QStringList() << "Ctrl+Shift+6" );
        shortcuts.emplace( LogViewAddColorLabel7, QStringList() << "Ctrl+Shift+7" );
        shortcuts.emplace( LogViewAddColorLabel8, QStringList() << "Ctrl+Shift+8" );
        shortcuts.emplace( LogViewAddColorLabel9, QStringList() << "Ctrl+Shift+9" );
        shortcuts.emplace( LogViewClearColorLabels, QStringList() << "Ctrl+Shift+0" );
        shortcuts.emplace( LogViewAddNextColorLabel, QStringList() << "Ctrl+D" );

        shortcuts.emplace( LogViewSendSelectionToScratchpad, QStringList() << "Ctrl+Z" );
        shortcuts.emplace( LogViewReplaceScratchpadWithSelection, QStringList() << "Ctrl+Shift+Z" );

        shortcuts.emplace( LogViewAddToSearch, QStringList() << "Shift+A" );
        shortcuts.emplace( LogViewExcludeFromSearch, QStringList() << "Shift+E" );
        shortcuts.emplace( LogViewReplaceSearch, QStringList() << "Shift+R" );

        shortcuts.emplace( LogViewSelectLinesUp, QStringList() << "Shift+Up" );
        shortcuts.emplace( LogViewSelectLinesDown, QStringList() << "Shift+Down" );

        return shortcuts;
    }();

    return defaultShortcuts;
}

QStringList ShortcutAction::defaultShortcuts( const std::string& action )
{
    const auto& shortcuts = defaultShortcuts();
    const auto actionShortcuts = shortcuts.find( action );
    if ( actionShortcuts == shortcuts.end() ) {
        return {};
    }

    return actionShortcuts->second;
}

QString ShortcutAction::actionName( const std::string& action )
{
    static const std::map<std::string, QString> actionNames = []() {
        std::map<std::string, QString> shortcuts;

        shortcuts.emplace( MainWindowNewWindow, QApplication::tr( "Open new window" ) );
        shortcuts.emplace( MainWindowOpenFile, QApplication::tr( "Open file" ) );
        shortcuts.emplace( MainWindowCloseFile, QApplication::tr( "Close file" ) );
        shortcuts.emplace( MainWindowCloseAll, QApplication::tr( "Close all files" ) );
        shortcuts.emplace( MainWindowSelectAll, QApplication::tr( "Select all" ) );
        shortcuts.emplace( MainWindowCopy, QApplication::tr( "Copy selection to clipboard" ) );
        shortcuts.emplace( MainWindowQuit, QApplication::tr( "Exit application" ) );
        shortcuts.emplace( MainWindowOpenQf, QApplication::tr( "Open quick find" ) );
        shortcuts.emplace( MainWindowOpenQfForward, QApplication::tr( "Quick find forward" ) );
        shortcuts.emplace( MainWindowOpenQfBackward, QApplication::tr( "Quick find backward" ) );
        shortcuts.emplace( MainWindowFocusSearchInput,
                           QApplication::tr( "Set focus to search input" ) );
        shortcuts.emplace( MainWindowClearFile, QApplication::tr( "Clear file" ) );
        shortcuts.emplace( MainWindowOpenContainingFolder,
                           QApplication::tr( "Open containing folder" ) );
        shortcuts.emplace( MainWindowOpenInEditor, QApplication::tr( "Open file in editor" ) );
        shortcuts.emplace( MainWindowCopyPathToClipboard,
                           QApplication::tr( "Copy file path to clipboard" ) );
        shortcuts.emplace( MainWindowOpenFromClipboard,
                           QApplication::tr( "Paste text from clipboard" ) );
        shortcuts.emplace( MainWindowOpenFromUrl, QApplication::tr( "Open file from URL" ) );
        shortcuts.emplace( MainWindowFollowFile, QApplication::tr( "Monitor file changes" ) );
        shortcuts.emplace( MainWindowReload, QApplication::tr( "Reload file" ) );
        shortcuts.emplace( MainWindowStop, QApplication::tr( "Stop file loading" ) );
        shortcuts.emplace( MainWindowScratchpad, QApplication::tr( "Open scratchpad" ) );
        shortcuts.emplace( MainWindowSelectOpenFile, QApplication::tr( "Switch to file" ) );

        shortcuts.emplace( CrawlerChangeVisibility,
                           QApplication::tr( "Change filtered lines visibility" ) );
        shortcuts.emplace( CrawlerIncreseTopViewSize, QApplication::tr( "Increase main view" ) );
        shortcuts.emplace( CrawlerDecreaseTopViewSize, QApplication::tr( "Decrease main view" ) );

        shortcuts.emplace( QfFindNext, QApplication::tr( "QuickFind: Find next" ) );
        shortcuts.emplace( QfFindPrev, QApplication::tr( "QuickFind: Find previous" ) );

        shortcuts.emplace( LogViewMark, QApplication::tr( "Add line mark" ) );

        shortcuts.emplace( LogViewNextMark, QApplication::tr( "Jump to next mark" ) );
        shortcuts.emplace( LogViewPrevMark, QApplication::tr( "Jump to previous mark" ) );
        shortcuts.emplace( LogViewSelectionUp, QApplication::tr( "Move selection up" ) );
        shortcuts.emplace( LogViewSelectionDown, QApplication::tr( "Move selection down" ) );
        shortcuts.emplace( LogViewScrollUp, QApplication::tr( "Scroll up" ) );
        shortcuts.emplace( LogViewScrollDown, QApplication::tr( "Scroll down" ) );
        shortcuts.emplace( LogViewScrollLeft, QApplication::tr( "Scroll left" ) );
        shortcuts.emplace( LogViewScrollRight, QApplication::tr( "Scroll right" ) );
        shortcuts.emplace( LogViewJumpToStartOfLine,
                           QApplication::tr( "Jump to the beginning of the current line" ) );
        shortcuts.emplace( LogViewJumpToEndOfLine,
                           QApplication::tr( "Jump to the end start of the current line" ) );
        shortcuts.emplace( LogViewJumpToRightOfScreen,
                           QApplication::tr( "Jump to the right of the text" ) );
        shortcuts.emplace( LogViewJumpToButtom,
                           QApplication::tr( "Jump to the buttom of the text" ) );
        shortcuts.emplace( LogViewJumpToTop, QApplication::tr( "Jump to the top of the text" ) );
        shortcuts.emplace( LogViewJumpToLine, QApplication::tr( "Jump to line" ) );
        shortcuts.emplace( LogViewQfForward, QApplication::tr( "Main view: find next" ) );
        shortcuts.emplace( LogViewQfBackward, QApplication::tr( "Main view: find previous" ) );

        shortcuts.emplace( LogViewQfSelectedForward,
                           QApplication::tr( "Set selection to QuickFind and find next" ) );
        shortcuts.emplace( LogViewQfSelectedBackward,
                           QApplication::tr( "Set selection to QuickFind and find previous" ) );

        shortcuts.emplace( LogViewExitView, QApplication::tr( "Release focus from view" ) );

        shortcuts.emplace( LogViewAddColorLabel1,
                           QApplication::tr( "Highlight text with color 1" ) );
        shortcuts.emplace( LogViewAddColorLabel2,
                           QApplication::tr( "Highlight text with color 2" ) );
        shortcuts.emplace( LogViewAddColorLabel3,
                           QApplication::tr( "Highlight text with color 3" ) );
        shortcuts.emplace( LogViewAddColorLabel4,
                           QApplication::tr( "Highlight text with color 4" ) );
        shortcuts.emplace( LogViewAddColorLabel5,
                           QApplication::tr( "Highlight text with color 5" ) );
        shortcuts.emplace( LogViewAddColorLabel6,
                           QApplication::tr( "Highlight text with color 6" ) );
        shortcuts.emplace( LogViewAddColorLabel7,
                           QApplication::tr( "Highlight text with color 7" ) );
        shortcuts.emplace( LogViewAddColorLabel8,
                           QApplication::tr( "Highlight text with color 8" ) );
        shortcuts.emplace( LogViewAddColorLabel9,
                           QApplication::tr( "Highlight text with color 9" ) );

        shortcuts.emplace( LogViewAddNextColorLabel,
                           QApplication::tr( "Highlight text with next color" ) );

        shortcuts.emplace( LogViewClearColorLabels, QApplication::tr( "Clear all color labels" ) );

        shortcuts.emplace( LogViewSendSelectionToScratchpad,
                           QApplication::tr( "Send selection to scratchpad" ) );
        shortcuts.emplace( LogViewReplaceScratchpadWithSelection,
                           QApplication::tr( "Replace scratchpad with selection" ) );

        shortcuts.emplace( LogViewAddToSearch,
                           QApplication::tr( "Add selection to search pattern" ) );
        shortcuts.emplace( LogViewExcludeFromSearch,
                           QApplication::tr( "Exclude selection from search pattern " ) );
        shortcuts.emplace( LogViewReplaceSearch,
                           QApplication::tr( "Replace search pattern with selection" ) );

        shortcuts.emplace( LogViewSelectLinesUp, QApplication::tr( "Select lines down" ) );
        shortcuts.emplace( LogViewSelectLinesDown, QApplication::tr( "Select lines up" ) );

        return shortcuts;
    }();

    const auto name = actionNames.find( action );

    return name != actionNames.end() ? name->second : QString::fromStdString( action );
}

void ShortcutAction::registerShortcut( const ConfiguredShortcuts& configuredShortcuts,
                                       std::map<QString, QShortcut*>& shortcutsStorage,
                                       QWidget* shortcutsParent, Qt::ShortcutContext context,
                                       const std::string& action,
                                       const std::function<void()>& func )
{
    const auto keysConfiguration = configuredShortcuts.find( action );
    const auto keys = keysConfiguration != configuredShortcuts.end()
                          ? keysConfiguration->second
                          : ShortcutAction::defaultShortcuts( action );

    for ( const auto& key : qAsConst( keys ) ) {
        if ( key.isEmpty() ) {
            continue;
        }

        auto shortcut = shortcutsStorage.extract( key );
        if ( shortcut ) {
            shortcut.mapped()->deleteLater();
        }

        registerShortcut( key, shortcutsStorage, shortcutsParent, context, func );
    }
}

void ShortcutAction::registerShortcut( const QString& key,
                                       std::map<QString, QShortcut*>& shortcutsStorage,
                                       QWidget* shortcutsParent, Qt::ShortcutContext context,
                                       const std::function<void()>& func )
{
    auto newShortcut = new QShortcut( QKeySequence( key ), shortcutsParent );
    newShortcut->setContext( context );
    newShortcut->connect( newShortcut, &QShortcut::activated, shortcutsParent,
                          [ func ] { func(); } );
    shortcutsStorage.emplace( key, newShortcut );
}

QList<QKeySequence> ShortcutAction::shortcutKeys( const std::string& action,
                                                  const ConfiguredShortcuts& configuredShortcuts )
{
    const auto keysConfiguration = configuredShortcuts.find( action );
    const auto keys = keysConfiguration != configuredShortcuts.end()
                          ? keysConfiguration->second
                          : ShortcutAction::defaultShortcuts( action );

    QList<QKeySequence> shortcuts;
    std::transform( keys.cbegin(), keys.cend(), std::back_inserter( shortcuts ),
                    []( const QString& hotkeys ) { return QKeySequence( hotkeys ); } );

    return shortcuts;
}