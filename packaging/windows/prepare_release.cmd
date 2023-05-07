echo %KLOGG_QT%
echo %KLOGG_QT_DIR%

md %KLOGG_WORKSPACE%\release

echo "Copying klogg binaries..."
xcopy %KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\output\klogg_portable.exe %KLOGG_WORKSPACE%\release\ /y
xcopy %KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\output\klogg_portable.pdb %KLOGG_WORKSPACE%\release\ /y
xcopy %KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\output\klogg.exe %KLOGG_WORKSPACE%\release\ /y
xcopy %KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\output\klogg.pdb %KLOGG_WORKSPACE%\release\ /y

xcopy %KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\output\klogg_crashpad_handler.exe %KLOGG_WORKSPACE%\release\ /y
xcopy %KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\output\klogg_minidump_dump.exe %KLOGG_WORKSPACE%\release\ /y

xcopy %KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\output\mimalloc.dll %KLOGG_WORKSPACE%\release\ /y
xcopy %KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\output\mimalloc.pdb %KLOGG_WORKSPACE%\release\ /y
xcopy %KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\output\mimalloc-redirect.dll %KLOGG_WORKSPACE%\release\ /y
xcopy %KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\output\mimalloc-redirect32.dll %KLOGG_WORKSPACE%\release\ /y

xcopy %KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\msvc_19.34_cxx17_64_md_relwithdebinfo\tbb12.dll %KLOGG_WORKSPACE%\release\ /y
xcopy %KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\msvc_19.34_cxx17_64_md_relwithdebinfo\tbb12.pdb %KLOGG_WORKSPACE%\release\ /y
xcopy %KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\msvc_19.34_cxx17_64_md_relwithdebinfo\tbbmalloc.dll %KLOGG_WORKSPACE%\release\ /y
xcopy %KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\msvc_19.34_cxx17_64_md_relwithdebinfo\tbbmalloc.pdb %KLOGG_WORKSPACE%\release\ /y
xcopy %KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\msvc_19.34_cxx17_64_md_relwithdebinfo\tbbmalloc_proxy.dll %KLOGG_WORKSPACE%\release\ /y
xcopy %KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\msvc_19.34_cxx17_64_md_relwithdebinfo\tbbmalloc_proxy.pdb %KLOGG_WORKSPACE%\release\ /y

xcopy %KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\msvc_19.34_cxx17_32_md_relwithdebinfo\tbb12.dll %KLOGG_WORKSPACE%\release\ /y
xcopy %KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\msvc_19.34_cxx17_32_md_relwithdebinfo\tbb12.pdb %KLOGG_WORKSPACE%\release\ /y
xcopy %KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\msvc_19.34_cxx17_32_md_relwithdebinfo\tbbmalloc.dll %KLOGG_WORKSPACE%\release\ /y
xcopy %KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\msvc_19.34_cxx17_32_md_relwithdebinfo\tbbmalloc.pdb %KLOGG_WORKSPACE%\release\ /y
xcopy %KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\msvc_19.34_cxx17_32_md_relwithdebinfo\tbbmalloc_proxy.dll %KLOGG_WORKSPACE%\release\ /y
xcopy %KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\msvc_19.34_cxx17_32_md_relwithdebinfo\tbbmalloc_proxy.pdb %KLOGG_WORKSPACE%\release\ /y

xcopy %KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\generated\documentation.html %KLOGG_WORKSPACE%\release\ /y
xcopy %KLOGG_WORKSPACE%\COPYING %KLOGG_WORKSPACE%\release\ /y
xcopy %KLOGG_WORKSPACE%\NOTICE %KLOGG_WORKSPACE%\release\ /y
xcopy %KLOGG_WORKSPACE%\README.md %KLOGG_WORKSPACE%\release\ /y
xcopy %KLOGG_WORKSPACE%\DOCUMENTATION.md %KLOGG_WORKSPACE%\release\ /y

echo "Copying vc runtime..."
xcopy "%VCToolsRedistDir%%platform%\Microsoft.VC143.CRT\msvcp140.dll" %KLOGG_WORKSPACE%\release\ /y
xcopy "%VCToolsRedistDir%%platform%\Microsoft.VC143.CRT\msvcp140_1.dll" %KLOGG_WORKSPACE%\release\ /y
xcopy "%VCToolsRedistDir%%platform%\Microsoft.VC143.CRT\msvcp140_2.dll" %KLOGG_WORKSPACE%\release\ /y
xcopy "%VCToolsRedistDir%%platform%\Microsoft.VC143.CRT\vcruntime140.dll" %KLOGG_WORKSPACE%\release\ /y
xcopy "%VCToolsRedistDir%%platform%\Microsoft.VC143.CRT\vcruntime140_1.dll" %KLOGG_WORKSPACE%\release\ /y

echo "Copying ssl..."
xcopy %SSL_DIR%\libcrypto-1_1%SSL_ARCH%.dll %KLOGG_WORKSPACE%\release\ /y
xcopy %SSL_DIR%\libssl-1_1%SSL_ARCH%.dll %KLOGG_WORKSPACE%\release\ /y

echo "Copying Qt..."
set "QTDIR=%KLOGG_QT_DIR:/=\%"
echo %QTDIR%
xcopy %QTDIR%\bin\%KLOGG_QT%Core.dll %KLOGG_WORKSPACE%\release\ /y
xcopy %QTDIR%\bin\%KLOGG_QT%Gui.dll %KLOGG_WORKSPACE%\release\ /y
xcopy %QTDIR%\bin\%KLOGG_QT%Network.dll %KLOGG_WORKSPACE%\release\ /y
xcopy %QTDIR%\bin\%KLOGG_QT%Widgets.dll %KLOGG_WORKSPACE%\release\ /y
xcopy %QTDIR%\bin\%KLOGG_QT%Concurrent.dll %KLOGG_WORKSPACE%\release\ /y
xcopy %QTDIR%\bin\%KLOGG_QT%Xml.dll %KLOGG_WORKSPACE%\release\ /y
xcopy %QTDIR%\bin\%KLOGG_QT%Core5Compat.dll %KLOGG_WORKSPACE%\release\ /y

md %KLOGG_WORKSPACE%\release\platforms
xcopy %QTDIR%\plugins\platforms\qwindows.dll %KLOGG_WORKSPACE%\release\platforms\ /y

md %KLOGG_WORKSPACE%\release\styles
xcopy %QTDIR%\plugins\styles\qwindowsvistastyle.dll %KLOGG_WORKSPACE%\release\styles /y


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