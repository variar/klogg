echo %KLOGG_QT%
echo %KLOGG_QT_DIR%

md %KLOGG_WORKSPACE%\release

echo "Copying klogg binaries..."
xcopy %KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\output\klogg_portable.exe %KLOGG_WORKSPACE%\release\ /y
xcopy %KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\output\klogg_portable.pdb %KLOGG_WORKSPACE%\release\ /y
xcopy %KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\output\klogg.exe %KLOGG_WORKSPACE%\release\ /y
xcopy %KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\output\klogg.pdb %KLOGG_WORKSPACE%\release\ /y


rem oneTBB puts its dll either into the common output folder
rem or into a folder named after MSVC version, e.g. msvc_19.51_cxx17_64_md_relwithdebinfo
if exist %KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\output\tbb12.dll (
  xcopy %KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\output\tbb12.dll %KLOGG_WORKSPACE%\release\ /y
  xcopy %KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\output\tbb12.pdb %KLOGG_WORKSPACE%\release\ /y
)
for /d %%D in (%KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\msvc_*_md_relwithdebinfo) do (
  xcopy %%D\tbb12.dll %KLOGG_WORKSPACE%\release\ /y
  xcopy %%D\tbb12.pdb %KLOGG_WORKSPACE%\release\ /y
)
if not exist %KLOGG_WORKSPACE%\release\tbb12.dll (
  echo "tbb12.dll not found"
  exit /b 1
)

xcopy %KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\generated\documentation.html %KLOGG_WORKSPACE%\release\ /y
xcopy %KLOGG_WORKSPACE%\COPYING %KLOGG_WORKSPACE%\release\ /y
xcopy %KLOGG_WORKSPACE%\NOTICE %KLOGG_WORKSPACE%\release\ /y
xcopy %KLOGG_WORKSPACE%\README.md %KLOGG_WORKSPACE%\release\ /y
xcopy %KLOGG_WORKSPACE%\DOCUMENTATION.md %KLOGG_WORKSPACE%\release\ /y

echo "Copying vc runtime..."
rem CRT folder name depends on Visual Studio version (VC143, VC145, ...)
for /d %%C in ("%VCToolsRedistDir%%platform%\Microsoft.VC*.CRT") do (
  xcopy "%%C\msvcp140.dll" %KLOGG_WORKSPACE%\release\ /y
  xcopy "%%C\msvcp140_1.dll" %KLOGG_WORKSPACE%\release\ /y
  xcopy "%%C\msvcp140_2.dll" %KLOGG_WORKSPACE%\release\ /y
  xcopy "%%C\vcruntime140.dll" %KLOGG_WORKSPACE%\release\ /y
  xcopy "%%C\vcruntime140_1.dll" %KLOGG_WORKSPACE%\release\ /y
)
if not exist %KLOGG_WORKSPACE%\release\vcruntime140.dll (
  echo "VC runtime not found in %VCToolsRedistDir%%platform%"
  exit /b 1
)

echo "Copying Qt..."
set "QTDIR=%KLOGG_QT_DIR:/=\%"
echo %QTDIR%
if not exist %QTDIR%\bin\%KLOGG_QT%Core.dll (
  echo "Qt not found in '%QTDIR%'"
  exit /b 1
)
xcopy %QTDIR%\bin\%KLOGG_QT%Core.dll %KLOGG_WORKSPACE%\release\ /y
xcopy %QTDIR%\bin\%KLOGG_QT%Gui.dll %KLOGG_WORKSPACE%\release\ /y
xcopy %QTDIR%\bin\%KLOGG_QT%Network.dll %KLOGG_WORKSPACE%\release\ /y
xcopy %QTDIR%\bin\%KLOGG_QT%Widgets.dll %KLOGG_WORKSPACE%\release\ /y
xcopy %QTDIR%\bin\%KLOGG_QT%Concurrent.dll %KLOGG_WORKSPACE%\release\ /y
xcopy %QTDIR%\bin\%KLOGG_QT%Xml.dll %KLOGG_WORKSPACE%\release\ /y
xcopy %QTDIR%\bin\%KLOGG_QT%Core5Compat.dll %KLOGG_WORKSPACE%\release\ /y

md %KLOGG_WORKSPACE%\release\platforms
xcopy %QTDIR%\plugins\platforms\qwindows.dll %KLOGG_WORKSPACE%\release\platforms\ /y

rem TLS backends: native Windows schannel (no OpenSSL needed) and certificate-only fallback
md %KLOGG_WORKSPACE%\release\tls
xcopy %QTDIR%\plugins\tls\qschannelbackend.dll %KLOGG_WORKSPACE%\release\tls\ /y
xcopy %QTDIR%\plugins\tls\qcertonlybackend.dll %KLOGG_WORKSPACE%\release\tls\ /y
if not exist %KLOGG_WORKSPACE%\release\tls\qschannelbackend.dll (
  echo "Qt schannel TLS backend not found"
  exit /b 1
)

md %KLOGG_WORKSPACE%\release\styles
xcopy %QTDIR%\plugins\styles\qwindowsvistastyle.dll %KLOGG_WORKSPACE%\release\styles /y
xcopy %QTDIR%\plugins\styles\qmodernwindowsstyle.dll %KLOGG_WORKSPACE%\release\styles /y

echo "Copying packaging files..."
md %KLOGG_WORKSPACE%\chocolately
xcopy %KLOGG_WORKSPACE%\packaging\windows\klogg.nuspec chocolately /y

md %KLOGG_WORKSPACE%\chocolately\tools
xcopy %KLOGG_WORKSPACE%\packaging\windows\chocolatelyInstall.ps1 chocolately\tools\ /y

xcopy %KLOGG_WORKSPACE%\packaging\windows\klogg.nsi  /y
xcopy %KLOGG_WORKSPACE%\packaging\windows\FileAssociation.nsh  /y

echo "Making portable archive..."
7z a -r %KLOGG_WORKSPACE%\klogg-%KLOGG_VERSION%-%KLOGG_ARCH%-%KLOGG_QT%-portable.zip @%KLOGG_WORKSPACE%\packaging\windows\7z_klogg_listfile.txt
7z a %KLOGG_WORKSPACE%\klogg-%KLOGG_VERSION%-%KLOGG_ARCH%-%KLOGG_QT%-pdb.zip @%KLOGG_WORKSPACE%\packaging\windows\7z_pdb_listfile.txt

echo "Done!"
