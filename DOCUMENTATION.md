# Klogg documentation

## Table of Contents

1. [Getting started](#Getting-started)
1. [Exploring log files](#Exploring-log-files)
1. [Settings](#Settings)
1. [Keyboard commands](#Keyboard-commands)
1. [Command line options](#Command-line-options)


## Getting started

*klogg* can be started from the command line, optionally passing the
file to open as an argument, or via the desktop environment's menu or
file association. If no file name is passed, *klogg* will initially open
the most recent file.

The main window is divided into three parts: the top displays the log
file. The bottom part, called the "filtered view", displays the results of
the search. The line separating the two contains the regular expression
used as a filter.

Entering a new regular expression or a simple search term will update
the bottom view, displaying the results of the search. The lines
matching the search criteria are listed in order in the results, and are
marked with a red circle in both windows.

## Exploring log files

Regular expressions are a powerful way to extract the information you
want from the log file. *klogg* uses *extended regular
expressions*.

One of the most useful regexp features when exploring logs is the
*alternation* feature, using parentheses and the | operator. It searches for
several alternatives and displays several line types in the
filtered window, in the same order they appear in the log file.

For example, to verify that every connection opened is also closed, one
can use an expression similar to:

`Entering (Open|Close)Connection`

Any 'open' call without a matching 'close' will immediately stand out
in the filtered window. The alternation also works with the whole search
line. If you would like to know what kind of connection has
been opened:

`Entering (Open|Close)Connection|Created a .* connection`

`.*` will match any sequence of characters on a single line, but *klogg*
will only display lines with a space and the word `connection` somewhere
after `Created a`

Sometimes alternation using regular expression syntax is cumbersome.
For such cases *klogg* can do logical search pattern combinations using
`and`, `or`, and `not` operators. This mode can be enabled using button
from search input panel. In this mode all patterns must be enclosed in `"`.
Following logic operations are supported:

|Operator        |Actions                                                                   |
|----------------|--------------------------------------------------------------------------|
|`and`           |Logical AND, True only if x and y both match input line. (eg: `"x" and "y"`)|
|`or`            |Logical OR, True if either x or y match input line. (eg: `"x" or "y"`)      |
|`&`             |Similar to AND but with left to right expression short circuiting optimization  |
|`\|`             |Similar to OR but with left to right expression short circuiting optimization   |
|`not`           |Logical NOT, Negate the logical sense of the input. Input must be enclosed in `()` (eg: `not("x")`)|

*klogg* keeps track of used search patterns and provides autocomplete
for them. This history can be edited or cleared from the search text box context menu.
Autocomplete is case-sensitive if this option is selected for matching 
regular expressions. The size of autocomplete history is configured in general options.

In addition to the filtered window, the match overview on the right-hand
side of the screen offers a view of the position of matches in the log
file. Matches are shown as small red lines.

In addition to regexp matches, *klogg* enables its users to mark any
interesting line in the log. To do this, click on the round bullet in
the left margin in front of the line that needs to be marked. Or, select
the line and press the `'m'` hotkey.
To mark several lines at once select them and use the `'m'` hotkey or context menu.

By default, filtered view always shows all marked lines. It is possible to switch filtered
view mode to show either only the lines matching search pattern or only marked lines.

Marks also appear as blue lines in the match overview.

It is possible to quickly jump to a specific line using `Ctrl+L` shortcut.

*klogg* uses Hyperscan library to perform regular expressions search. Hyperscan is very
fast, but it doesn't support some patterns, most notably any lookahead is not supported 
(check [hyperscan documentation](https://intel.github.io/hyperscan/dev-reference/compilation.html#pattern-support) for 
supported syntax). To overcome this *klogg* will switch to Qt regular expression engine with full PCRE syntax support
if Hyperscan can't handle the search pattern. However, in this case search will be significantly slower.

### Opening files

*klogg* provides several options for opening files:

* using dedicated open file item in `File` menu or toolbar
* dragging files from the file manager
* downloading files from a provided url
* providing one or many files via the command line
* using recent files or favorite menu items.

On Windows and Mac OS, the *klogg* installer configures the operating system to open `.log` files by
clicking them in the file manager.

#### Archives

*klogg* can open archives (`zip`, `7z`, and `tar`). The archive is extracted
to a temporary directory and standard open file dialog is presented to
select files. The type of archive is determined automatically by file
content or extension.

*klogg* can open compressed files (`gzip`, `bzip2`, `xz`, `lzma`). Such files are
decompressed to a temporary folder and then opened. The compression type is
determined automatically by file content or extension.

#### Remote URLs

*klogg* can open files from remote URLs. In that case, *klogg* will
download the file to a temporary directory and open it from there.

#### Recent files

*klogg* saves a history of recent opened files. Up to 5 recent files are
available from the `File` menu.

#### Favorites

Opened files can be added to the `Favorites` menu either from
`Favorites->Add to Favorites` or from the toolbar.

This menu is used to provide fast access to files that are opened less
often and don't end up in the recent files section.

#### Clipboard

Pasting text from the clipboard to *klogg* also works. In this case, *klogg*
will save pasted text to a temporary file and open that file for
exploring.

#### Switching between opened files

Switching from one opened file to another can be done from the
`View->Opened files` menu or by using the `Ctrl+Shift+O` shortcut 
which displays special dialogue to choose between opened files.

### Encodings

*klogg* tries to guess the encoding of an opened file. If that guess happens to
be wrong, then the desired encoding can be selected from the `Encoding` menu.

### Predefined filters

If some search patterns are used very often they can be saved as predefined filters.
Predefined filters are configured from the `Tools` menu.

Predefined filters are added to a dropdown near the search input and allow to 
add several patterns to regular expression. Predefined filter has a name
which is displayed in the dropdown, a pattern to add to search regular expression
and a setting to treat pattern as a regular expression
or simple text search.

It is possible to save the current search pattern as a predefined filter from
search input context menu.

### Using highlighters

*Highlighters* can colorize some lines of the log being displayed
to draw attention to lines indicating an error, or to associate
a color with a certain type of event. 

Highlighters are grouped into sets. One set of highlighters can be active
at any given time. The current active set can be selected using either the
context menu or the `Tools->Highlighters` menu.

Any number of highlighters can be defined in a single set.
Highlighter configuration includes a regular expression to match
as well as color options. Another option is to use plain text patterns
in cases when complex regular expression are unnecessary.
Highlighters don't have support for logical search pattern combinations.

Each highlighter can be configured to apply foreground and 
background colors either to the whole line that matched its regular
expression or only to matching parts of the line. In the latter case,
if the regular expression contains capture groups then only the captured
parts of the matching line are highlighted.

It is possible to set a color variance. In that case different strings
that match the same regular expression will have slightly different color.

Any number of highlighters set can be applied to opened file using either 
the context menu or the main menu.

The order of highlighters in the set and the order of sets in configuration is important.
For each line all highlighters are tried from bottom to top. Each new matching 
highlighter overrides colors for the current line. 

Highlighter configuration can be exported to a file and 
imported on another machine. Each set is identified
by unique id. Only new sets are imported from the file. Please export the file
with a `.conf` extension to ensure *klogg* will be able to import it.

### Color labels

In addition to predefined highlighters sets it is possible to create quick highlight rules
from selected text. These are called color labels. By default, *klogg* has 9
color labels enabled. Adding color label to selected text is done either
via context menu or with shortcuts `Ctrl+Shift+1-9`. Any number of 
strings can be marked with a single color label. Also the is `Ctrl+D` shortcut
that applies the next color label to selected text.

To remove color label from selected text either select the text and use 
context menu to set color label to `None` or use `Ctrl+Shift+0` shortcut that
will remove all color labels.

The colors that are used for text highlight can be configured from the color labels
tab of highlighters configuration dialog.

### Browsing changing log files

*klogg* can display and search through logs while they are written to
a disk. This might be the case when debugging a running program or
server. The log is automatically updated when it grows, but the
'Auto-refresh' option must be enabled if you want the search results to
be automatically refreshed.

The `'f'` key may be used to follow the end of the file as it grows (a
la `tail -f`).

*klogg* detects if new lines have been appended to the file or if the file has
been overwritten. In the former case, search results will be updated as new
matching lines appear in the file. If the file is overwritten, then
search results will be cleared. 

*klogg* has two options to distinguish appends from overwrites.
The general and more stable option is to recalculate the hash of the 
indexed part of the file and check if it matches current file on disk. 
This is reliable but can be slow for large files and for slow file systems
(e.g. network shares). The other option is to check hashes for only the 
first and last parts of the file. This usually works quickly 
but can skip over changes in the middle of the file. You can choose your 
preferred option in `Settings->File` tab.

The following file mode requires monitoring of the file system for any changes.
If native monitoring or polling are both disabled in settings, then the 
following file mode is also disabled.

### Scratchpad

Sometimes in log files there are text in base64 encoding, unformatted
xml/json, etc. For such cases *klogg* provides Scratchpad tool. Text can
be copied to this window and transformed to human-readable form.
Use context menu to either add data to the current scratchpad tab or 
replace its content with selected text. There are shortcuts `Ctrl+Z` and `Ctrl+Shit+Z` for these actions.

New tabs can be opened in Scratchpad using the `Ctrl+N` hotkey.

## Settings

### General

#### Search options

Determines which type of regular expression *klogg* will use when
filtering lines for the bottom window, and when using QuickFind.

*   Extended Regexp. The default, uses regular expressions similar to
    those used by Perl
*   Fixed Strings. Searches for the text exactly as it is written, no
    character is special

If incremental quickfind is selected, *klogg* will automatically restart
quickfind search when the search pattern changes.

Turning on highlight of matched text will cause the text that matched the
search pattern to be highlighted in both main view and filtered view.
Enabling color variation will cause the highlight color of different strings
that match the same pattern be slightly different.

Search size history controls the number of patterns that are saved for autocompletion
in the search input box.

Turning on option to run search on add or replace pattern will cause *klogg* to
immediately perform search when pattern is update from context menu.

#### Session options

*   Load last session -- if enabled, *klogg* will reopen files that were
    opened when *klogg* was closed. View configuration, marked lines and
    `follow` mode settings are restored for each file.
*   Follow file on load -- if enabled, *klogg* will enter `follow` mode
    for for all new opened files.
*   Minimize to tray -- if enabled, *klogg* will minimize to tray instead
    of closing main window. Use tray icon context menu of `File->Exit`
    to exit application. This option is not available on Mac OS.
*   Enable multiple windows -- if enabled *klogg* will allow opening
    more than one main window using `File->New window`. In this mode last
    closed windows will be saved to open session on next *klogg* start.
    When exiting *klogg* using `File->Exit` all windows are saved and
    will be reopened.

#### Version checking options

If version checking is enabled then *klogg* will try to grab a version
information file from the GitHub repository and see if a new version has been released
once per week.

Stable builds will check if a new stable version is available and pop a dialogue about it.
Testing builds will check for new testing versions.

### View

#### Font

The font used to display the log file. A clear, monospace font (like the
free, open source, [DejaVu Mono](http://www.dejavu-fonts.org) for
example, is recommended.

Font antialiasing can be forced if auto-detected options result in low-quality
text rendering.

Font size can be changed from either main or filtered view using `Ctrl+Mouse wheel`
to zoom in/out.

#### Style

Qt usually comes with several options for drawing application widgets.
By default, *klogg* uses a style that matches current operating systems.
Other styles can be chosen from the dropdown menu.

*klogg* will try to respect current display manager theme and to
use white icons for dark themes. 

Another option is to select Dark or Windows Dark style. In this case *klogg*
will use a custom dark mode stylesheet. 

#### High DPI

Options in this group can be used in case *klogg* window looks 
bad on High DPI monitors. Usually, Qt detects the correct settings.
However, these options may be useful, especially for non-integer
scale factors manual overrides.

#### Miscellaneous

Some log files contain ANSI color codes to be displayed by terminals with
color support. These color codes create visual noise, so *klogg* provides
an option to hide them from both main and filtered view. However, enabling
this option will cause regular expression search to be slower.

### File

#### File change monitoring

If file change monitoring is enabled, *klogg* will use facilities
provided by the operating system to reload the file when data is changed on the
disk.

Sometimes this kind of monitoring is unreliable on
network shares or directories mounted via sftp. In that case, polling can
be enabled to make *klogg* check for changes.

*klogg* tries to detect if the file was changed in the already indexed
area. This mechanism involves hash recalculation and can be slow for
large files and network filesystems. If fast modification detection
is enabled *klogg* will check hash for the first and last parts of
changed files. This is faster but can skip over changes in the middle of
the file. This feature should be used with caution.

It is possible to enable follow file mode by scrolling past the end of file.
This behavior can be disabled.

#### Encoding

*klogg* tries to detect file encoding automatically. If encoding detection
is not required then it is possible to specify the encoding that will be
used for all new opened files.

#### Archives

If extract archives is selected then *klogg* will detect if opened file
is of one of supported archives type or a single compressed file and
will ask user permission to extract archives content to a temporary folder.

If you do not want *klogg* to ask for permission, check 
"extract archives without confirmation" option.

#### File download

By default, *klogg* will not download files using HTTPS if certificates
can't be checked. In some development environments self-signed 
certificates are used. In this case, *klogg* can be instructed to ignore
SSL errors.

### Advanced options

These options refer to the customization of performance related settings.

If parallel search is enabled, *klogg* will try to use several CPU cores
for regular expression matching. This does not work with quickfind.

*klogg* has several strategies for regular expression search based on file 
encoding. By default, it is optimized for files with UTF8 or single-byte
encodings. If most of the files are in multi-byte encodings then enabling
search optimization for non-latin encodings could improve performance.

If search results cache is enabled, *klogg* will store numbers of lines
that matched the search pattern in its memory. Repeating searches for the same
pattern will not go through all files but will use cached line numbers
instead.

In case there is an issue with *klogg*, logging can be enabled with
a desired level of verbosity. Log files are saved to a temporary directory.
A log level of 4 or 5 is usually enough. Enabling logging can slow down 
regular expressions search.

## Crash reporting

*klogg* uses Crashpad crash handler to collect minidump files in case of 
unexpected crashes. At startup, *klogg* checks for new minidumps and asks  the user
if these files should be sent to developers.

Crash report provides information about:

* operating system: name, version, architecture, cpu features, system memory
* Qt version
* modules that were loaded into *klogg* process: filename, size and hashes for symbols
* stacktraces for all running threads in *klogg* process

These minidumps do not include the full content of *klogg* process memory during the crash.

## Keyboard commands

*klogg* keyboard commands try to approximately emulate the default
bindings used by the classic Unix utilities *vi* and *less*.

The main commands are:

|Keys            |Actions                                                           |
|----------------|------------------------------------------------------------------|
|arrows          |scroll one line up/down or one column left/right                  |
|\[number\] j/k  |move the selection 'number' (or one) line down/up                 |
|h/l             |scroll left/right                                                 |
|\^ or \$        |scroll to beginning or end of selected line                       |
|\[number\] g    |jump to the line number given or the first one if no number is    |
|                |entered                                                           |
|G               |jump to the first line of the file (selecting it)                 |
|Shift+G         |jump to the last line of the file (selecting it)                  |
|Alt+G           |show jump to line dialog                                          |
|' or "          |start a quickfind search in the current screen                    |
|                |(forward and backward)                                            |
|n or N          |repeat the previous quickfind search forward/backward             |
|\* or .         |search for the next occurrence of the currently selected text      |
|/ or ,          |search for the previous occurrence of the currently selected text  |
|f               |activate 'follow' mode, which keep the display as the tail of the |
|                |file (like "tail -f")                                             |
|m               |put a mark on current selected line                               |
|\[ or \]        |jump to previous or next marked line                              |
|+ or -          |decrease/increase filtered view size                              |
|v               |switch filtered view visibility mode                               |
|                |(Marks and Matches -&gt; Marks -&gt; Matches)                     |
|F5              |reload current file                                               |
|Ctrl+S          |Set focus to search string edit box                               |
|Ctrl+Shift+O    |Open dialog to switch to another file                             |

All shortucts can be configured from the shortcuts tab in options dialog.

## Mouse navigation

Holding `Alt` while scrolling will scroll horizontally.
Holding `Shift` while scrolling will scroll faster.

## Command line options

|Switch             |Actions                                                   |
|-------------------|----------------------------------------------------------|
|-h, --help         |print help message and exit                               |
|-v, --version      |print version information                                 |
|-m,--multi         |allow multiple instance of klogg to run simultaneously (use together with -s)|                                    |
|-s,--load-session  |load the previous session (default when no file is passed)|
|-n,--new-session   |do not load the previous session (default when a file is passed) |
|-l,--log           |save the log to a file                                    |
|-f,--follow        |follow initial opened files                               |
|-d,--debug         |output more debug (include multiple times for more verbosity e.g. -dddd) |

