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
        shortcuts.emplace( LogViewReplaceScratchpadWithSelection, QStringList() << "Ctrl+Shift+Z");
        
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

        shortcuts.emplace( MainWindowNewWindow, "Open new window" );
        shortcuts.emplace( MainWindowOpenFile, "Open file" );
        shortcuts.emplace( MainWindowCloseFile, "Close file" );
        shortcuts.emplace( MainWindowCloseAll, "Close all files" );
        shortcuts.emplace( MainWindowSelectAll, "Select all" );
        shortcuts.emplace( MainWindowCopy, "Copy selection to clipboard" );
        shortcuts.emplace( MainWindowQuit, "Exit application" );
        shortcuts.emplace( MainWindowOpenQf, "Open quick find" );
        shortcuts.emplace( MainWindowOpenQfForward, "Quick find forward" );
        shortcuts.emplace( MainWindowOpenQfBackward, "Quick find backward" );
        shortcuts.emplace( MainWindowFocusSearchInput, "Set focus to search input" );
        shortcuts.emplace( MainWindowClearFile, "Clear file" );
        shortcuts.emplace( MainWindowOpenContainingFolder, "Open containgin folder" );
        shortcuts.emplace( MainWindowOpenInEditor, "Open file in editor" );
        shortcuts.emplace( MainWindowCopyPathToClipboard, "Copy file path to clipboard" );
        shortcuts.emplace( MainWindowOpenFromClipboard, "Paste text from clipboard" );
        shortcuts.emplace( MainWindowOpenFromUrl, "Open file from URL" );
        shortcuts.emplace( MainWindowFollowFile, "Monitor file changes" );
        shortcuts.emplace( MainWindowReload, "Reload file" );
        shortcuts.emplace( MainWindowStop, "Stop file loading" );
        shortcuts.emplace( MainWindowScratchpad, "Open scratchpad" );
        shortcuts.emplace( MainWindowSelectOpenFile, "Switch to file" );

        shortcuts.emplace( CrawlerChangeVisibility, "Change filtered lines visibility" );
        shortcuts.emplace( CrawlerIncreseTopViewSize, "Increase main view" );
        shortcuts.emplace( CrawlerDecreaseTopViewSize, "Decrease main view" );

        shortcuts.emplace( QfFindNext, "QuickFind: Find next" );
        shortcuts.emplace( QfFindPrev, "QuickFind: Find previous" );

        shortcuts.emplace( LogViewMark, "Add line mark" );

        shortcuts.emplace( LogViewNextMark, "Jump to next mark" );
        shortcuts.emplace( LogViewPrevMark, "Jump to previous mark" );
        shortcuts.emplace( LogViewSelectionUp, "Move selection up" );
        shortcuts.emplace( LogViewSelectionDown, "Move selection down" );
        shortcuts.emplace( LogViewScrollUp, "Scroll up" );
        shortcuts.emplace( LogViewScrollDown, "Scroll down" );
        shortcuts.emplace( LogViewScrollLeft, "Scroll left" );
        shortcuts.emplace( LogViewScrollRight, "Scroll right" );
        shortcuts.emplace( LogViewJumpToStartOfLine, "Jump to the beginning of the current line" );
        shortcuts.emplace( LogViewJumpToEndOfLine, "Jump to the end start of the current line" );
        shortcuts.emplace( LogViewJumpToRightOfScreen, "Jump to the right of the text" );
        shortcuts.emplace( LogViewJumpToButtom, "Jump to the buttom of the text" );
        shortcuts.emplace( LogViewJumpToTop, "Jump to the top of the text" );
        shortcuts.emplace( LogViewJumpToLine, "Jump to line" );
        shortcuts.emplace( LogViewQfForward, "Main view: find next" );
        shortcuts.emplace( LogViewQfBackward, "Main view: find previous" );

        shortcuts.emplace( LogViewQfSelectedForward, "Set selection to QuickFind and find next" );
        shortcuts.emplace( LogViewQfSelectedBackward,
                           "Set selection to QuickFind and find previous" );

        shortcuts.emplace( LogViewExitView, "Release focus from view" );

        shortcuts.emplace( LogViewAddColorLabel1, "Highlight text with color 1" );
        shortcuts.emplace( LogViewAddColorLabel2, "Highlight text with color 2" );
        shortcuts.emplace( LogViewAddColorLabel3, "Highlight text with color 3" );
        shortcuts.emplace( LogViewAddColorLabel4, "Highlight text with color 4" );
        shortcuts.emplace( LogViewAddColorLabel5, "Highlight text with color 5" );
        shortcuts.emplace( LogViewAddColorLabel6, "Highlight text with color 6" );
        shortcuts.emplace( LogViewAddColorLabel7, "Highlight text with color 7" );
        shortcuts.emplace( LogViewAddColorLabel8, "Highlight text with color 8" );
        shortcuts.emplace( LogViewAddColorLabel9, "Highlight text with color 9" );

        shortcuts.emplace( LogViewAddNextColorLabel, "Highlight text with next color" );

        shortcuts.emplace( LogViewClearColorLabels, "Clear all color labels" );

        shortcuts.emplace( LogViewSendSelectionToScratchpad, "Send selection to scratchpad" );
        shortcuts.emplace( LogViewReplaceScratchpadWithSelection, "Replace scratchpad with selection" );

        shortcuts.emplace( LogViewAddToSearch, "Add selection to search pattern" );
        shortcuts.emplace( LogViewExcludeFromSearch, "Exclude selection from search pattern " );
        shortcuts.emplace( LogViewReplaceSearch, "Replace search pattern with selection" );

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

void ShortcutAction::registerShortcut( const QString key,
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