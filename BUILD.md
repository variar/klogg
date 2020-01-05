# Overview

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes.

# Dependencies

* cmake 3.12 or later to generate build files
* C++ compiler with C++14 support (gcc 5, clang 3.4, msvc 2017)
* Qt libraries 5.9 or later:
    - QtCore
    - QtGui
    - QtWidgets
    - QtConcurrent
    - QtNetwork
    - QtXml
    - QtTest
* pandoc to build documentation (optional)
* nsis to build windows installer (optional)

All other dependencies are provided in 3rdparty directory.

# Getting the Source

This project is [hosted on GitHub](https://github.com/variar/klogg). You can clone this project directly using this command:
```
git clone https://github.com/variar/klogg
```

# Building

## Building on Linux

Here is how to build klogg on Ubuntu 18.04.

Install dependencies:
```
sudo apt-get install build-essential cmake qtbase5-dev
```

Configure and build klogg:

```
cd <path_to_klogg_repository_clone>
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_DOC=False ..
cmake --build .
```

Binaries are placed into `build/output`.

See `.travis.yml` for more information on build process.

## Building on Windows

Install Microsoft Visual Studio 2017 or 2019 with C++ support.
Community edition can be downloaded from [Microsoft](https://visualstudio.microsoft.com/vs/).

Intall latest Qt version using [online installer](https://www.qt.io/download-qt-installer).
Make sure to select version matching Visual Studio installation. 64-bit libraries are recommended.

Install CMake from [Kitware](https://cmake.org/download/).
Use version 3.14 or later for Visual Studio 2019 support.

Prepare build environment for CMake. Open command prompt window and depending on version of Visual Studio run either
```
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\Common7\Tools\vsdevcmd" -arch=x64
```
or
```
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\Common7\Tools\vsdevcmd" -arch=x64
```

Next setup Qt paths:
```
<path_to_qt_installation>\bin\qtenv2.bat
```

Then add CMake to PATH:
```
set PATH=<path_to_cmake_bin>:$PATH
```

Configure klogg solution (use CMake generator matching Visual Studio version):
```
cd <path_to_project_root>
md build
cd build
cmake -G "Visual Studio 16 2019 Win64" -DCMAKE_BUILD_TYPE=Release -DBUILD_DOC=False ..
```

CMake should generate `klogg.sln` file in `<path_to_project_root>\build` directory. Open solution and build it.

Binaries are placed into `build/output`.

## Building on Mac OS

Klogg requires macOS High Sierra (10.13) or higher.

Install [Homebrew](https://brew.sh/) using terminal:
```
/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
```

Homebrew installer should also install xcode command line tools.

Download and install build dependencies:
```
brew install cmake ninja qt
```

Configure and build klogg:
```
cd <path_to_klogg_repository_clone>
mkdir build
cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_DOC=False -DQt5_DIR=/usr/local/Cellar/qt/5.14.0/lib/cmake/Qt5 ..
cmake --build .
```

Binaries are placed into `build/output`.

# Running tests
Tests are built by default. To turn them off pass `-DBUILD_TESTS:BOOL=OFF` to cmake.
Tests use catch2 (bundled with klogg sources) and require Qt5Test module. Tests can be run using ctest tool provider by CMake:
```
cd <path_to_klogg_repository_clone>
cd build
ctest --build-config Release --verbose
```
