# NSIS script creating the Windows installer for klogg

# Is passed to the script using -DVERSION=$(git describe) on the command line
!ifndef VERSION
    !define VERSION 'dev-build'
!endif

!ifndef PLATFORM
    !define PLATFORM 'unknown'
!endif

!ifndef QT_MAJOR
    !define QT_MAJOR 'Qt5'
!endif

# Headers
!include "MUI2.nsh"
!include "FileAssociation.nsh"

# General
OutFile "klogg-${VERSION}-${PLATFORM}-${QT_MAJOR}-setup.exe"

XpStyle on

SetCompressor /SOLID lzma

; Registry key to keep track of the directory we are installed in
!ifdef ARCH32
  InstallDir "$PROGRAMFILES\klogg"
!else
  InstallDir "$PROGRAMFILES64\klogg"
!endif
InstallDirRegKey HKLM Software\klogg ""

; klogg icon
; !define MUI_ICON klogg.ico

RequestExecutionLevel admin

Name "klogg"
Caption "klogg ${VERSION} Setup"

# Pages
!define MUI_WELCOMEPAGE_TITLE "Welcome to the klogg ${VERSION} Setup Wizard"
!define MUI_WELCOMEPAGE_TEXT "This wizard will guide you through the installation of klogg\
, a fast, advanced log explorer.$\r$\n$\r$\n\
klogg and the Qt libraries are released under the GPL, see \
the COPYING and NOTICE files.$\r$\n$\r$\n$_CLICK"
;MUI_FINISHPAGE_LINK_LOCATION "https://klogg.filimonov.dev/"

!insertmacro MUI_PAGE_WELCOME
;!insertmacro MUI_PAGE_LICENSE "COPYING"
# !ifdef VER_MAJOR & VER_MINOR & VER_REVISION & VER_BUILD...
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

# Languages
!insertmacro MUI_LANGUAGE "English"

# Installer sections
Section "klogg" klogg
    ; Prevent this section from being unselected
    SectionIn RO

    SetOutPath $INSTDIR
    File release\klogg.exe
    File release\klogg_crashpad_handler.exe
    File release\klogg_minidump_dump.exe
    File release\tbb12.dll
    File release\tbbmalloc.dll
    File release\tbbmalloc_proxy.dll

    File COPYING
    File NOTICE
    File README.md
    File DOCUMENTATION.md
    File release\documentation.html

    ; Create the 'sendto' link
    CreateShortCut "$SENDTO\klogg.lnk" "$INSTDIR\klogg,exe" "" "$INSTDIR\klogg.exe" 0

    ; Register as an otion (but not main handler) for some files (.txt, .Log, .cap)
    WriteRegStr HKCR "Applications\klogg.exe" "" ""
    WriteRegStr HKCR "Applications\klogg.exe\shell" "" "open"
    WriteRegStr HKCR "Applications\klogg.exe\shell\open" "Klogg log viewer" "klogg"
    WriteRegStr HKCR "Applications\klogg.exe\shell\open\command" "" '"$INSTDIR\klogg.exe" "%1"'
    WriteRegStr HKCR "*\OpenWithList\klogg.exe" "" ""
    WriteRegStr HKCR ".txt\OpenWithList\klogg.exe" "" ""
    WriteRegStr HKCR ".Log\OpenWithList\klogg.exe" "" ""
    WriteRegStr HKCR ".cap\OpenWithList\klogg.exe" "" ""

    ; Register uninstaller
    WriteRegExpandStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\klogg"\
"UninstallString" '"$INSTDIR\Uninstall.exe"'
    WriteRegExpandStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\klogg"\
"InstallLocation" "$INSTDIR"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\klogg" "DisplayName" "klogg"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\klogg" "DisplayVersion" "${VERSION}"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\klogg" "NoModify" "1"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\klogg" "NoRepair" "1"

    ; Create uninstaller
    WriteUninstaller "$INSTDIR\Uninstall.exe"
SectionEnd

Section "Qt Runtime libraries" qtlibs
    SetOutPath $INSTDIR
    File release\${QT_MAJOR}Core.dll
    File release\${QT_MAJOR}Gui.dll
    File release\${QT_MAJOR}Network.dll
    File release\${QT_MAJOR}Widgets.dll
    File release\${QT_MAJOR}Concurrent.dll
    File release\${QT_MAJOR}Xml.dll
!if ${QT_MAJOR} == "Qt6"
    File release\${QT_MAJOR}Core5Compat.dll
!endif

    SetOutPath $INSTDIR\platforms
    File release\platforms\qwindows.dll
    SetOutPath $INSTDIR\styles
    File release\styles\qwindowsvistastyle.dll
SectionEnd

Section "MSVC Runtime libraries" vcruntime
    SetOutPath $INSTDIR
    File release\msvcp140.dll
    File release\msvcp140_1.dll
    File release\vcruntime140.dll
    
!if ${PLATFORM} == "x64"
    File release\vcruntime140_1.dll

    File release\libcrypto-1_1-x64.dll
    File release\libssl-1_1-x64.dll
!else
    File release\libcrypto-1_1.dll
    File release\libssl-1_1.dll
!endif

SectionEnd

Section "Create Start menu shortcut" shortcut
    SetShellVarContext all
    CreateShortCut "$SMPROGRAMS\klogg.lnk" "$INSTDIR\klogg.exe" "" "$INSTDIR\klogg.exe" 0
SectionEnd

Section /o "Associate with .log files" associate
    ${registerExtension} "$INSTDIR\klogg.exe" ".log" "Log file"
SectionEnd

# Descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${klogg} "The core files required to use klogg."
    !insertmacro MUI_DESCRIPTION_TEXT ${qtlibs} "Needed by klogg, you have to install these unless \
you already have the Qt development kit installed."
    !insertmacro MUI_DESCRIPTION_TEXT ${vcruntime} "Needed by klogg, you have to install these unless \
you already have the Microsoft Visual C++ 2017 Redistributable installed."
    !insertmacro MUI_DESCRIPTION_TEXT ${shortcut} "Create a shortcut in the Start menu for klogg."
    !insertmacro MUI_DESCRIPTION_TEXT ${associate} "Make klogg the default viewer for .log files."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

# Uninstaller
Section "Uninstall"
    Delete "$INSTDIR\Uninstall.exe"

    Delete "$INSTDIR\klogg.exe"
    Delete "$INSTDIR\klogg_crashpad_handler.exe"
    Delete "$INSTDIR\klogg_minidump_dump.exe"
    Delete "$INSTDIR\README.md"
    Delete "$INSTDIR\COPYING"
    Delete "$INSTDIR\NOTICE"
    Delete "$INSTDIR\readme.html"
    Delete "$INSTDIR\documentation.md"
    Delete "$INSTDIR\documentation.html"
    Delete "$INSTDIR\libstdc++-6.dll"
    Delete "$INSTDIR\libgcc_s_seh-1.dll"
    Delete "$INSTDIR\libgcc_s_dw2-1.dll"
    Delete "$INSTDIR\Qt5Widgets.dll"
    Delete "$INSTDIR\Qt5Core.dll"
    Delete "$INSTDIR\Qt5Gui.dll"
    Delete "$INSTDIR\Qt5Network.dll"
    Delete "$INSTDIR\Qt5Concurrent.dll"
    Delete "$INSTDIR\Qt5Xml.dll"
    Delete "$INSTDIR\Qt6Widgets.dll"
    Delete "$INSTDIR\Qt6Core.dll"
    Delete "$INSTDIR\Qt6Gui.dll"
    Delete "$INSTDIR\Qt6Network.dll"
    Delete "$INSTDIR\Qt6Concurrent.dll"
    Delete "$INSTDIR\Qt6Xml.dll"
    Delete "$INSTDIR\Qt6Core5Compat.dll"
    Delete "$INSTDIR\platforms\qwindows.dll"
    Delete "$INSTDIR\platforms\qminimal.dll"
    Delete "$INSTDIR\styles\qwindowsvistastyle.dll"
    Delete "$INSTDIR\msvcp140.dll"
    Delete "$INSTDIR\msvcp140_1.dll"
    Delete "$INSTDIR\vcruntime140.dll"
    Delete "$INSTDIR\vcruntime140_1.dll"
    Delete "$INSTDIR\tbb12.dll"
    Delete "$INSTDIR\tbbmalloc.dll"
    Delete "$INSTDIR\tbbmalloc_proxy.dll"
    Delete "$INSTDIR\klogg_tbbmalloc.dll"
    Delete "$INSTDIR\klogg_tbbmalloc_proxy.dll"
    Delete "$INSTDIR\libcrypto-1_1-x64.dll"
    Delete "$INSTDIR\libssl-1_1-x64.dll"
    Delete "$INSTDIR\libcrypto-1_1.dll"
    Delete "$INSTDIR\libssl-1_1.dll"
    Delete "$INSTDIR\mimalloc.dll"
    Delete "$INSTDIR\mimalloc_override.dll"
    Delete "$INSTDIR\mimalloc_redirect.dll"
    Delete "$INSTDIR\mimalloc_redirect32.dll"
    Delete "$INSTDIR\mimalloc-redirect.dll"
    Delete "$INSTDIR\mimalloc-redirect32.dll"
    RMDir "$INSTDIR"

    ; Remove settings in %appdata%
    Delete "$APPDATA\klogg\klogg.ini"
    RMDir "$APPDATA\klogg"

    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\klogg"

    ; Remove the file associations
    ${unregisterExtension} ".log" "Log file"

    DeleteRegKey HKCR "*\OpenWithList\klogg.exe"
    DeleteRegKey HKCR ".txt\OpenWithList\klogg.exe"
    DeleteRegKey HKCR ".Log\OpenWithList\klogg.exe"
    DeleteRegKey HKCR ".cap\OpenWithList\klogg.exe"
    DeleteRegKey HKCR "Applications\klogg.exe\shell\open\command"
    DeleteRegKey HKCR "Applications\klogg.exe\shell\open"
    DeleteRegKey HKCR "Applications\klogg.exe\shell"
    DeleteRegKey HKCR "Applications\klogg.exe"

    ; Remove the shortcut, if any
    SetShellVarContext all
    Delete "$SMPROGRAMS\klogg.lnk"
SectionEnd

;!uninstfinalize 'packaging\windows\codesign_client.exe --debug "%1"'
