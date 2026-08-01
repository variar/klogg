@echo off
setlocal

echo KLOGG_QT=%KLOGG_QT%
echo KLOGG_QT_DIR=%KLOGG_QT_DIR%

md %KLOGG_WORKSPACE%\release

echo "Copying klogg binaries..."
xcopy /Y /Q "%KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\output\klogg_portable.exe" "%KLOGG_WORKSPACE%\release\" 2>nul
xcopy /Y /Q "%KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\output\klogg_portable.pdb" "%KLOGG_WORKSPACE%\release\" 2>nul
xcopy /Y /Q "%KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\output\klogg.exe" "%KLOGG_WORKSPACE%\release\" 2>nul
xcopy /Y /Q "%KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\output\klogg.pdb" "%KLOGG_WORKSPACE%\release\" 2>nul

xcopy /Y /Q "%KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\output\klogg_crashpad_handler.exe" "%KLOGG_WORKSPACE%\release\" 2>nul
xcopy /Y /Q "%KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\output\klogg_minidump_dump.exe" "%KLOGG_WORKSPACE%\release\" 2>nul

REM Copy TBB DLLs - try multiple possible paths and also search recursively
for %%P in ("msvc_19.41_cxx17_64_md_relwithdebinfo" "msvc_19.42_cxx17_64_md_relwithdebinfo" "msvc_19.41_cxx17_64_md_relwithdebinfo_merged_typeinfo" "msvc_19.42_cxx17_64_md_relwithdebinfo_merged_typeinfo") do (
    if exist "%KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\%%~P\tbb12.dll" (
        xcopy /Y /Q "%KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\%%~P\tbb12.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
        xcopy /Y /Q "%KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\%%~P\tbb12.pdb" "%KLOGG_WORKSPACE%\release\" 2>nul
    )
)
for %%P in ("msvc_19.41_cxx17_32_md_relwithdebinfo" "msvc_19.42_cxx17_32_md_relwithdebinfo" "msvc_19.41_cxx17_32_md_relwithdebinfo_merged_typeinfo" "msvc_19.42_cxx17_32_md_relwithdebinfo_merged_typeinfo") do (
    if exist "%KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\%%~P\tbb12.dll" (
        xcopy /Y /Q "%KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\%%~P\tbb12.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
        xcopy /Y /Q "%KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\%%~P\tbb12.pdb" "%KLOGG_WORKSPACE%\release\" 2>nul
    )
)

REM Fallback: search recursively in build_root for TBB DLLs
for /r "%KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%" %%F in (tbb12.dll) do (
    if exist "%%F" (
        copy /Y "%%F" "%KLOGG_WORKSPACE%\release\" 2>nul
    )
)
for /r "%KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%" %%F in (tbb12.pdb) do (
    if exist "%%F" (
        copy /Y "%%F" "%KLOGG_WORKSPACE%\release\" 2>nul
    )
)
for /r "%KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%" %%F in (tbbmalloc.dll) do (
    if exist "%%F" (
        copy /Y "%%F" "%KLOGG_WORKSPACE%\release\" 2>nul
    )
)
for /r "%KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%" %%F in (tbbmalloc_proxy.dll) do (
    if exist "%%F" (
        copy /Y "%%F" "%KLOGG_WORKSPACE%\release\" 2>nul
    )
)

xcopy /Y /Q "%KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\generated\documentation.html" "%KLOGG_WORKSPACE%\release\" 2>nul
xcopy /Y /Q "%KLOGG_WORKSPACE%\COPYING" "%KLOGG_WORKSPACE%\release\" 2>nul
xcopy /Y /Q "%KLOGG_WORKSPACE%\NOTICE" "%KLOGG_WORKSPACE%\release\" 2>nul
xcopy /Y /Q "%KLOGG_WORKSPACE%\README.md" "%KLOGG_WORKSPACE%\release\" 2>nul
xcopy /Y /Q "%KLOGG_WORKSPACE%\DOCUMENTATION.md" "%KLOGG_WORKSPACE%\release\" 2>nul

REM Determine platform for VC runtime (x64 or x86)
if "%VSCMD_ARG_TGT_ARCH%"=="x64" (
    set "PLATFORM_DIR=x64"
) else (
    set "PLATFORM_DIR=x86"
)

REM If VCToolsRedistDir is not set, try to find it
if "%VCToolsRedistDir%"=="" (
    for /d %%R in ("C:\Program Files (x86)\Microsoft Visual Studio\*\*\VC\Redist") do (
        set "VCToolsRedistDir=%%R\"
    )
)

echo "Copying vc runtime from %VCToolsRedistDir%..."
if exist "%VCToolsRedistDir%%PLATFORM_DIR%\Microsoft.VC143.CRT\msvcp140.dll" (
    xcopy /Y /Q "%VCToolsRedistDir%%PLATFORM_DIR%\Microsoft.VC143.CRT\msvcp140.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
    xcopy /Y /Q "%VCToolsRedistDir%%PLATFORM_DIR%\Microsoft.VC143.CRT\msvcp140_1.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
    xcopy /Y /Q "%VCToolsRedistDir%%PLATFORM_DIR%\Microsoft.VC143.CRT\msvcp140_2.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
    xcopy /Y /Q "%VCToolsRedistDir%%PLATFORM_DIR%\Microsoft.VC143.CRT\vcruntime140.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
    xcopy /Y /Q "%VCToolsRedistDir%%PLATFORM_DIR%\Microsoft.VC143.CRT\vcruntime140_1.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
)

REM Try alternative CRT locations for Windows 2022
if not exist "%KLOGG_WORKSPACE%\release\msvcp140.dll" (
    for /d %%C in ("C:\Program Files (x86)\Microsoft Visual Studio\**\VC\Redist") do (
        xcopy /Y /Q "%%C\%PLATFORM_DIR%\Microsoft.VC143.CRT\msvcp140.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
        xcopy /Y /Q "%%C\%PLATFORM_DIR%\Microsoft.VC143.CRT\msvcp140_1.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
        xcopy /Y /Q "%%C\%PLATFORM_DIR%\Microsoft.VC143.CRT\vcruntime140.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
        xcopy /Y /Q "%%C\%PLATFORM_DIR%\Microsoft.VC143.CRT\vcruntime140_1.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
    )
)

echo "Copying ssl..."
REM Calculate SSL_DIR based on KLOGG_WORKSPACE and KLOGG_ARCH
set "SSL_DIR=%KLOGG_WORKSPACE%\openssl-1.1\%KLOGG_ARCH%\bin"
REM Convert forward slashes to backslashes
set "SSL_DIR=%SSL_DIR:/=\%"
echo SSL_DIR=%SSL_DIR%
if exist "%SSL_DIR%" (
    if "%KLOGG_ARCH%"=="x64" (
        xcopy /Y /Q "%SSL_DIR%\libcrypto-1_1-x64.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
        xcopy /Y /Q "%SSL_DIR%\libssl-1_1-x64.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
    ) else (
        xcopy /Y /Q "%SSL_DIR%\libcrypto-1_1.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
        xcopy /Y /Q "%SSL_DIR%\libssl-1_1.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
    )
)

echo "Deploying Qt from %KLOGG_QT_DIR%..."
set "QTDIR=%KLOGG_QT_DIR:/=\%"
if "%QTDIR%"=="" (
    echo KLOGG_QT_DIR is not set, trying to find Qt...
    for /d %%Q in ("C:\Qt" "D:\a\_temp" "%LOCALAPPDATA%\Qt" "C:\hostedtoolcache\Qt") do (
        if exist "%%Q" (
            for /d %%V in ("%%Q\6.*") do (
                set "QTDIR=%%V"
            )
        )
    )
)
echo Using QTDIR=%QTDIR%

REM Find the Qt bin directory containing DLLs
set "QTBIN="
if exist "%QTDIR%\bin" (
    if exist "%QTDIR%\bin\Qt6Core.dll" (
        set "QTBIN=%QTDIR%\bin"
    )
)

REM Check if Qt6Core.dll exists directly in QTDIR (like in klogg installed directory)
if "%QTBIN%"=="" (
    if exist "%QTDIR%\Qt6Core.dll" (
        set "QTBIN=%QTDIR%"
    )
)

REM Check klogg installed directory as fallback
if "%QTBIN%"=="" (
    for %%K in ("D:\klogg" "%PROGRAMFILES%\klogg" "%PROGRAMFILES(X86)%\klogg") do (
        if exist "%%~K\Qt6Core.dll" (
            echo Found Qt DLLs in klogg installed directory at %%~K
            set "QTBIN=%%~K"
            set "KLOGG_INSTALLED_DIR=%%~K"
        )
    )
)

REM If not found, search recursively
if "%QTBIN%"=="" (
    for /d %%Q in ("C:\Qt" "D:\a\_temp" "%LOCALAPPDATA%\Qt" "C:\hostedtoolcache\Qt") do (
        if exist "%%Q" (
            for /r "%%Q" %%F in (Qt6Core.dll) do (
                if exist "%%F" (
                    set "QTBIN=%%~dpF"
                    set "QTDIR=%%~dpF.."
                )
            )
        )
    )
)

echo QTBIN=%QTBIN%

REM Copy Qt DLLs manually - use Qt6 for Qt6 builds
if "%QTBIN%" neq "" (
    if exist "%QTBIN%\Qt6Core.dll" (
        echo Copying Qt6 DLLs...
        xcopy /Y /Q "%QTBIN%\Qt6Core.dll" "%KLOGG_WORKSPACE%\release\" 2>nul || echo Failed to copy Qt6Core.dll
        xcopy /Y /Q "%QTBIN%\Qt6Gui.dll" "%KLOGG_WORKSPACE%\release\" 2>nul || echo Failed to copy Qt6Gui.dll
        xcopy /Y /Q "%QTBIN%\Qt6Network.dll" "%KLOGG_WORKSPACE%\release\" 2>nul || echo Failed to copy Qt6Network.dll
        xcopy /Y /Q "%QTBIN%\Qt6Widgets.dll" "%KLOGG_WORKSPACE%\release\" 2>nul || echo Failed to copy Qt6Widgets.dll
        xcopy /Y /Q "%QTBIN%\Qt6Concurrent.dll" "%KLOGG_WORKSPACE%\release\" 2>nul || echo Failed to copy Qt6Concurrent.dll
        xcopy /Y /Q "%QTBIN%\Qt6Xml.dll" "%KLOGG_WORKSPACE%\release\" 2>nul || echo Failed to copy Qt6Xml.dll
        xcopy /Y /Q "%QTBIN%\Qt6Core5Compat.dll" "%KLOGG_WORKSPACE%\release\" 2>nul || echo Qt6Core5Compat.dll not found, skipping
    ) else if exist "%QTBIN%\Qt5Core.dll" (
        echo Copying Qt5 DLLs...
        xcopy /Y /Q "%QTBIN%\Qt5Core.dll" "%KLOGG_WORKSPACE%\release\" 2>nul || echo Failed to copy Qt5Core.dll
        xcopy /Y /Q "%QTBIN%\Qt5Gui.dll" "%KLOGG_WORKSPACE%\release\" 2>nul || echo Failed to copy Qt5Gui.dll
        xcopy /Y /Q "%QTBIN%\Qt5Network.dll" "%KLOGG_WORKSPACE%\release\" 2>nul || echo Failed to copy Qt5Network.dll
        xcopy /Y /Q "%QTBIN%\Qt5Widgets.dll" "%KLOGG_WORKSPACE%\release\" 2>nul || echo Failed to copy Qt5Widgets.dll
        xcopy /Y /Q "%QTBIN%\Qt5Concurrent.dll" "%KLOGG_WORKSPACE%\release\" 2>nul || echo Failed to copy Qt5Concurrent.dll
        xcopy /Y /Q "%QTBIN%\Qt5Xml.dll" "%KLOGG_WORKSPACE%\release\" 2>nul || echo Failed to copy Qt5Xml.dll
    )
) else (
    echo Qt bin directory not found! Searching recursively...
    for /r "C:\Qt" %%F in (Qt6Core.dll Qt5Core.dll) do (
        if exist "%%F" (
            set "FOUND_QT_BIN=%%~dpF"
            for %%D in (Qt6 Qt5) do (
                if exist "%%~dpF%%DCore.dll" (
                    echo Found %%D DLLs at %%~dpF
                    xcopy /Y /Q "%%~dpF%%DCore.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
                    xcopy /Y /Q "%%~dpF%%DGui.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
                    xcopy /Y /Q "%%~dpF%%DNetwork.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
                    xcopy /Y /Q "%%~dpF%%DWidgets.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
                    xcopy /Y /Q "%%~dpF%%DConcurrent.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
                    xcopy /Y /Q "%%~dpF%%DXml.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
                    xcopy /Y /Q "%%~dpF%%DCore5Compat.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
                )
            )
        )
    )
    for /r "D:\a\_temp" %%F in (Qt6Core.dll Qt5Core.dll) do (
        if exist "%%F" (
            set "FOUND_QT_BIN=%%~dpF"
            for %%D in (Qt6 Qt5) do (
                if exist "%%~dpF%%DCore.dll" (
                    echo Found %%D DLLs at %%~dpF
                    xcopy /Y /Q "%%~dpF%%DCore.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
                    xcopy /Y /Q "%%~dpF%%DGui.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
                    xcopy /Y /Q "%%~dpF%%DNetwork.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
                    xcopy /Y /Q "%%~dpF%%DWidgets.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
                    xcopy /Y /Q "%%~dpF%%DConcurrent.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
                    xcopy /Y /Q "%%~dpF%%DXml.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
                    xcopy /Y /Q "%%~dpF%%DCore5Compat.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
                )
            )
        )
    )
    for /r "C:\hostedtoolcache\Qt" %%F in (Qt6Core.dll Qt5Core.dll) do (
        if exist "%%F" (
            set "FOUND_QT_BIN=%%~dpF"
            for %%D in (Qt6 Qt5) do (
                if exist "%%~dpF%%DCore.dll" (
                    echo Found %%D DLLs at %%~dpF
                    xcopy /Y /Q "%%~dpF%%DCore.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
                    xcopy /Y /Q "%%~dpF%%DGui.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
                    xcopy /Y /Q "%%~dpF%%DNetwork.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
                    xcopy /Y /Q "%%~dpF%%DWidgets.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
                    xcopy /Y /Q "%%~dpF%%DConcurrent.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
                    xcopy /Y /Q "%%~dpF%%DXml.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
                    xcopy /Y /Q "%%~dpF%%DCore5Compat.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
                )
            )
        )
    )
)

REM Also copy additional Qt6 DLLs that might be needed
if exist "%QTBIN%\Qt6Core.dll" (
    for %%F in (
        "Qt6DBus.dll"
        "Qt6OpenGL.dll"
        "Qt6PrintSupport.dll"
        "Qt6Sql.dll"
        "Qt6Svg.dll"
        "Qt6Test.dll"
        "Qt6OpenGLWidgets.dll"
        "Qt6Gamepad.dll"
        "Qt6Pdf.dll"
        "Qt6Positioning.dll"
        "Qt6Qml.dll"
        "Qt6QmlModels.dll"
        "Qt6QmlWorkerScript.dll"
        "Qt6Quick.dll"
        "Qt6QuickWidgets.dll"
        "Qt6ShaderTools.dll"
        "Qt63DCore.dll"
        "Qt63DRender.dll"
        "Qt63DInput.dll"
        "Qt63DLogic.dll"
        "Qt6Charts.dll"
        "Qt6DataVisualization.dll"
        "Qt6NetworkAuth.dll"
        "Qt6TextToSpeech.dll"
        "Qt6StateMachine.dll"
        "Qt6Speech.dll"
        "Qt6SerialPort.dll"
        "Qt6RemoteObjects.dll"
        "Qt6WebChannel.dll"
        "icudt72.dll"
        "icuin72.dll"
        "icuuc72.dll"
    ) do (
        if exist "%QTBIN%\%%~F" (
            xcopy /Y /Q "%QTBIN%\%%~F" "%KLOGG_WORKSPACE%\release\" 2>nul
        )
    )
)

REM Copy Qt plugins from known locations
md "%KLOGG_WORKSPACE%\release\platforms"
md "%KLOGG_WORKSPACE%\release\styles"

REM Try to find plugins directory
set "QT_PLUGINS="
if "%QTDIR%" neq "" (
    if exist "%QTDIR%\plugins" (
        set "QT_PLUGINS=%QTDIR%\plugins"
    )
)

REM Search for plugins if not found
if "%QT_PLUGINS%"=="" (
    for /d %%Q in ("C:\Qt" "D:\a\_temp" "%LOCALAPPDATA%\Qt" "C:\hostedtoolcache\Qt") do (
        if exist "%%Q" (
            for /d %%P in ("%%Q\6.*\plugins") do (
                set "QT_PLUGINS=%%P"
            )
        )
    )
)

echo QT_PLUGINS=%QT_PLUGINS%

REM Copy platforms plugin
if "%QT_PLUGINS%" neq "" (
    if exist "%QT_PLUGINS%\platforms\qwindows.dll" (
        echo Copying qwindows.dll...
        xcopy /Y /Q "%QT_PLUGINS%\platforms\qwindows.dll" "%KLOGG_WORKSPACE%\release\platforms\" 2>nul || echo Failed to copy qwindows.dll
    )
    if exist "%QT_PLUGINS%\styles\qmodernwindowsstyle.dll" (
        echo Copying qmodernwindowsstyle.dll...
        xcopy /Y /Q "%QT_PLUGINS%\styles\qmodernwindowsstyle.dll" "%KLOGG_WORKSPACE%\release\styles\" 2>nul || echo Failed to copy qmodernwindowsstyle.dll
    )
    if exist "%QT_PLUGINS%\styles\qwindowsvistastyle.dll" (
        echo Copying qwindowsvistastyle.dll...
        xcopy /Y /Q "%QT_PLUGINS%\styles\qwindowsvistastyle.dll" "%KLOGG_WORKSPACE%\release\styles\" 2>nul || echo Failed to copy qwindowsvistastyle.dll
    )
)

REM Search recursively for plugins if not found
if not exist "%KLOGG_WORKSPACE%\release\platforms\qwindows.dll" (
    echo Searching for qwindows.dll recursively...
    for /r "C:\Qt" %%F in (qwindows.dll) do (
        if exist "%%F" (
            echo Found qwindows.dll at %%F
            xcopy /Y /Q "%%F" "%KLOGG_WORKSPACE%\release\platforms\" 2>nul
        )
    )
    for /r "D:\a\_temp" %%F in (qwindows.dll) do (
        if exist "%%F" (
            echo Found qwindows.dll at %%F
            xcopy /Y /Q "%%F" "%KLOGG_WORKSPACE%\release\platforms\" 2>nul
        )
    )
    for /r "C:\hostedtoolcache\Qt" %%F in (qwindows.dll) do (
        if exist "%%F" (
            echo Found qwindows.dll at %%F
            xcopy /Y /Q "%%F" "%KLOGG_WORKSPACE%\release\platforms\" 2>nul
        )
    )
)

REM Verify Qt DLLs were copied
echo.
echo Verifying Qt DLLs in release folder:
dir "%KLOGG_WORKSPACE%\release\Qt*Core.dll" 2>nul || echo Qt*Core.dll NOT FOUND

echo "Copying packaging files..."
md "%KLOGG_WORKSPACE%\chocolately"
xcopy /Y /Q "%KLOGG_WORKSPACE%\packaging\windows\klogg.nuspec" "%KLOGG_WORKSPACE%\chocolately\" 2>nul

md "%KLOGG_WORKSPACE%\chocolately\tools"
xcopy /Y /Q "%KLOGG_WORKSPACE%\packaging\windows\chocolatelyInstall.ps1" "%KLOGG_WORKSPACE%\chocolately\tools\" 2>nul

xcopy /Y /Q "%KLOGG_WORKSPACE%\packaging\windows\klogg.nsi" "%KLOGG_WORKSPACE%\" 2>nul
xcopy /Y /Q "%KLOGG_WORKSPACE%\packaging\windows\FileAssociation.nsh" "%KLOGG_WORKSPACE%\" 2>nul

echo "Making portable archive using PowerShell..."
cd /d "%KLOGG_WORKSPACE%"

REM Use PowerShell for reliable archive creation
powershell -NoProfile -ExecutionPolicy Bypass -Command "if (Test-Path 'release') { Compress-Archive -Path 'release\*' -DestinationPath 'klogg-%KLOGG_VERSION%-%KLOGG_ARCH%-%KLOGG_QT%-portable.zip' -Force }"

if errorlevel 1 (
    echo PowerShell portable archive creation failed
    exit /b 1
)

REM Create PDB archive
powershell -NoProfile -ExecutionPolicy Bypass -Command "Get-ChildItem -Path 'release' -Filter '*.pdb' | Compress-Archive -DestinationPath 'klogg-%KLOGG_VERSION%-%KLOGG_ARCH%-%KLOGG_QT%-pdb.zip' -Force"

REM Verify portable.zip was created
if not exist "klogg-%KLOGG_VERSION%-%KLOGG_ARCH%-%KLOGG_QT%-portable.zip" (
    echo ERROR: portable.zip was not created
    exit /b 1
)

echo "Done!"
