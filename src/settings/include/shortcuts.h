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

#ifndef KLOGG_SHORTCUTS_H
#define KLOGG_SHORTCUTS_H

#include <map>
#include <string>

#include <QKeySequence>
#include <QStringList>

class QWidget;
class QShortcut;

struct ShortcutAction {
    static constexpr auto CrawlerChangeVisibilityForward = "crawler.change_visibility_type";
    static constexpr auto CrawlerChangeVisibilityBackward
        = "crawler.change_visibility_type_backward";
    static constexpr auto CrawlerChangeVisibilityToMarksAndMatches
        = "crawler.change_visibility_to_marks_and_matches";
    static constexpr auto CrawlerChangeVisibilityToMarks = "crawler.change_visibility_to_marks";
    static constexpr auto CrawlerChangeVisibilityToMatches = "crawler.change_visibility_to_matches";
    static constexpr auto CrawlerIncreseTopViewSize = "crawler.increase_top_view_size";
    static constexpr auto CrawlerDecreaseTopViewSize = "crawler.decrease_top_view_size";
    static constexpr auto CrawlerEnableCaseMatching = "crawler.enable_case_matching";
    static constexpr auto CrawlerEnableRegex = "crawler.enable_regex";
    static constexpr auto CrawlerEnableInverseMatching = "crawler.enable_inverse_matching";
    static constexpr auto CrawlerEnableRegexCombining = "crawler.enable_regex_combining";
    static constexpr auto CrawlerEnableAutoRefresh = "crawler.enable_auto_refresh";
    static constexpr auto CrawlerKeepResults = "crawler.enable_keep_results";

    static constexpr auto QfFindNext = "qf.find_next";
    static constexpr auto QfFindPrev = "qf.find_prev";

    static constexpr auto MainWindowOpenFile = "mainwindow.open_file";
    static constexpr auto MainWindowNewWindow = "mainwindow.new_window";
    static constexpr auto MainWindowCloseFile = "mainwindow.close_file";
    static constexpr auto MainWindowCloseAll = "mainwindow.close_all";
    static constexpr auto MainWindowQuit = "mainwindow.quit";
    static constexpr auto MainWindowFullScreen = "mainwindow.fullscreen";
    static constexpr auto MainWindowMax = "mainwindow.max_window";
    static constexpr auto MainWindowMin = "mainwindow.min_window";
    static constexpr auto MainWindowPreference = "mainwindow.preference";
    static constexpr auto MainWindowCopy = "mainwindow.copy_selection";
    static constexpr auto MainWindowSelectAll = "mainwindow.select_all";
    static constexpr auto MainWindowOpenQf = "mainwindow.open_qf";
    static constexpr auto MainWindowOpenQfForward = "mainwindow.open_qf_forward";
    static constexpr auto MainWindowOpenQfBackward = "mainwindow.open_qf_backward";
    static constexpr auto MainWindowFocusSearchInput = "qf.focus_search";
    static constexpr auto MainWindowClearFile = "mainwindow.clear_file";
    static constexpr auto MainWindowFollowFile = "mainwindow.follow_file";
    static constexpr auto MainWindowTextWrap = "mainwindow.text_wrap";
    static constexpr auto MainWindowReload = "mainwindow.reload";
    static constexpr auto MainWindowStop = "mainwindow.stop";
    static constexpr auto MainWindowScratchpad = "mainwindow.scratchpad";
    static constexpr auto MainWindowSelectOpenFile = "mainwindow.select_open_file";
    static constexpr auto MainWindowOpenContainingFolder = "mainwindow.open_containing_folder";
    static constexpr auto MainWindowOpenInEditor = "mainwindow.open_in_editor";
    static constexpr auto MainWindowCopyPathToClipboard = "mainwindow.copy_path_to_clipboard";
    static constexpr auto MainWindowOpenFromClipboard = "mainwindow.open_from_clipboard";
    static constexpr auto MainWindowOpenFromUrl = "mainwindow.open_from_url";

    static constexpr auto LogViewMark = "logview.mark";
    static constexpr auto LogViewNextMark = "logview.next_mark";
    static constexpr auto LogViewPrevMark = "logview.prev_mark";
    static constexpr auto LogViewSelectionUp = "logview.selection_up";
    static constexpr auto LogViewSelectionDown = "logview.selection_down";
    static constexpr auto LogViewScrollUp = "logview.scroll_up";
    static constexpr auto LogViewScrollDown = "logview.scroll_down";
    static constexpr auto LogViewScrollLeft = "logview.scroll_left";
    static constexpr auto LogViewScrollRight = "logview.scroll_right";
    static constexpr auto LogViewJumpToTop = "logview.jump_to_top";
    static constexpr auto LogViewJumpToButtom = "logview.jump_to_buttom";
    static constexpr auto LogViewJumpToBottom = "logview.jump_to_bottom";
    static constexpr auto LogViewJumpToStartOfLine = "logview.jump_to_start_of_line";
    static constexpr auto LogViewJumpToEndOfLine = "logview.jump_to_end_of_line";
    static constexpr auto LogViewJumpToRightOfScreen = "logview.jump_to_right";
    static constexpr auto LogViewJumpToLine = "logview.jump_to_line";
    static constexpr auto LogViewJumpToLineNumber = "logview.jump_to_line_number";

    static constexpr auto LogViewQfSelectedForward = "logview.qf_selected_forward";
    static constexpr auto LogViewQfSelectedBackward = "logview.qf_selected_backward";
    static constexpr auto LogViewQfForward = "logview.open_qf_forward";
    static constexpr auto LogViewQfBackward = "logview.open_qf_backward";

    static constexpr auto LogViewExitView = "logview.exit_view";

    static constexpr auto LogViewAddColorLabel1 = "logview.add_color_label_1";
    static constexpr auto LogViewAddColorLabel2 = "logview.add_color_label_2";
    static constexpr auto LogViewAddColorLabel3 = "logview.add_color_label_3";
    static constexpr auto LogViewAddColorLabel4 = "logview.add_color_label_4";
    static constexpr auto LogViewAddColorLabel5 = "logview.add_color_label_5";
    static constexpr auto LogViewAddColorLabel6 = "logview.add_color_label_6";
    static constexpr auto LogViewAddColorLabel7 = "logview.add_color_label_7";
    static constexpr auto LogViewAddColorLabel8 = "logview.add_color_label_8";
    static constexpr auto LogViewAddColorLabel9 = "logview.add_color_label_9";

    static constexpr auto LogViewClearColorLabels = "logview.clear_color_labels";
    static constexpr auto LogViewAddNextColorLabel = "logview.add_next_color_label";

    static constexpr auto LogViewSendSelectionToScratchpad = "logview.send_selection_to_scratchpad";
    static constexpr auto LogViewReplaceScratchpadWithSelection
        = "logview.replace_scratchpad_with_selection";

    static constexpr auto LogViewAddToSearch = "logview.add_to_search";
    static constexpr auto LogViewExcludeFromSearch = "logview.exclude_from_search";
    static constexpr auto LogViewReplaceSearch = "logview.replace_search";
    static constexpr auto LogViewSelectLinesUp = "logview.select_lines_up";
    static constexpr auto LogViewSelectLinesDown = "logview.select_lines_down";

private:
    struct ShortcutDesc {
        QString name;
        QStringList keySequence;
    };

public:
    using Shortcut = ShortcutDesc;
    using ShortcutList = std::map<std::string, Shortcut>;           // <action, shortcut>
    using ConfiguredShortcuts = std::map<std::string, QStringList>; // <action,keySequence>

    static const ShortcutList& defaultShortcutList();

    static void registerShortcut( const ConfiguredShortcuts& configuredShortcuts,
                                  std::map<QString, QShortcut*>& shortcutsStorage,
                                  QWidget* shortcutsParent, Qt::ShortcutContext context,
                                  const std::string& action, const std::function<void()>& func );

    static void registerShortcut( const QString& key,
                                  std::map<QString, QShortcut*>& shortcutsStorage,
                                  QWidget* shortcutsParent, Qt::ShortcutContext context,
                                  const std::function<void()>& func );

    static QList<QKeySequence> shortcutKeys( const std::string& action,
                                             const ConfiguredShortcuts& configuredShortcuts );

private:
    static QStringList defaultShortcutKeys( const std::string& action );
};

#endif
