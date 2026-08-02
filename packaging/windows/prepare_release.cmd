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
echo Using QTDIR=%QTDIR%

REM Try to find Qt using windeployqt first (most reliable)
if exist "%QTDIR%\bin\windeployqt.exe" (
    echo "Running windeployqt to deploy Qt runtime into release dir"
    "%QTDIR%\bin\windeployqt.exe" --no-compiler-runtime --dir "%KLOGG_WORKSPACE%\release" "%KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\output\klogg.exe"
    if errorlevel 1 (
        echo windeployqt failed, trying manual copy...
        for %%D in (Qt5 Qt6) do (
            if exist "%QTDIR%\bin\%%DCore.dll" (
                echo Copying %%D* DLLs...
                xcopy /Y /Q "%QTDIR%\bin\%%DCore.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
                xcopy /Y /Q "%QTDIR%\bin\%%DGui.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
                xcopy /Y /Q "%QTDIR%\bin\%%DNetwork.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
                xcopy /Y /Q "%QTDIR%\bin\%%DWidgets.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
                xcopy /Y /Q "%QTDIR%\bin\%%DConcurrent.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
                xcopy /Y /Q "%QTDIR%\bin\%%DXml.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
                xcopy /Y /Q "%QTDIR%\bin\%%DCore5Compat.dll" "%KLOGG_WORKSPACE%\release\" 2>nul
            )
        )
    )
) else (
    echo "windeployqt not found at %QTDIR%\bin\windeployqt.exe"
    echo "Trying to find Qt DLLs in common locations..."

    REM Search for Qt DLLs recursively in various locations
    set "QT_FOUND=0"

    for /d %%Q in ("C:\Qt" "D:\a\_temp" "%LOCALAPPDATA%\Qt" "C:\Users\runneradmin\AppData\Local\Qt") do (
        if exist "%%Q" (
            echo Searching in %%Q...
            for /r "%%Q" %%F in (windeployqt.exe) do (
                if exist "%%F" (
                    echo Found windeployqt at %%F
                    "%%F" --no-compiler-runtime --dir "%KLOGG_WORKSPACE%\release" "%KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\output\klogg.exe"
                    set "QT_FOUND=1"
                )
            )
        )
    )

    REM If Qt not found yet, search for DLLs directly
    if "%QT_FOUND%"=="0" (
        echo "Searching for Qt DLLs directly..."
        for /d %%Q in ("C:\Qt" "D:\a\_temp" "%LOCALAPPDATA%\Qt") do (
            for /r "%%Q" %%F in (Qt6Core.dll Qt5Core.dll) do (
                if exist "%%F" (
                    echo Found Qt DLL at %%F
                    set "FOUND_QT_BIN=%%~dpF"
                    for %%D in (Qt5 Qt6) do (
                        if exist "%%~dpF%%DCore.dll" (
                            echo Copying %%D* DLLs from %%~dpF
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
    )
)

REM Copy Qt plugins from known locations
md "%KLOGG_WORKSPACE%\release\platforms"
md "%KLOGG_WORKSPACE%\release\styles"

for /d %%Q in ("C:\Qt" "D:\a\_temp" "%LOCALAPPDATA%\Qt" "%QTDIR%") do (
    if exist "%%Q\plugins\platforms\qwindows.dll" (
        xcopy /Y /Q "%%Q\plugins\platforms\qwindows.dll" "%KLOGG_WORKSPACE%\release\platforms\" 2>nul
    )
    if exist "%%Q\plugins\styles\qwindowsvistastyle.dll" (
        xcopy /Y /Q "%%Q\plugins\styles\qwindowsvistastyle.dll" "%KLOGG_WORKSPACE%\release\styles\" 2>nul
    )
    if exist "%%Q\plugins\styles\qmodernwindowsstyle.dll" (
        xcopy /Y /Q "%%Q\plugins\styles\qmodernwindowsstyle.dll" "%KLOGG_WORKSPACE%\release\styles\" 2>nul
    )
)

REM Search recursively for plugins if not found
if not exist "%KLOGG_WORKSPACE%\release\platforms\qwindows.dll" (
    for /d %%Q in ("C:\Qt" "D:\a\_temp" "%LOCALAPPDATA%\Qt") do (
        for /r "%%Q" %%F in (qwindows.dll) do (
            if exist "%%F" (
                echo Found qwindows.dll at %%F
                copy /Y "%%F" "%KLOGG_WORKSPACE%\release\platforms\" 2>nul
            )
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
