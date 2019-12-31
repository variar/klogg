#-------------------------------------------------
#
# Project created by QtCreator 2018-04-10T04:25:55
#
#-------------------------------------------------

#################################
# karchive version: 5.43.0
#################################

TARGET = karchive
TEMPLATE = lib
CONFIG += c++11
QT -= gui
QT += core

DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += DEBAO_BUILDING_KDE5TIER1

win32: LIBS += -ladvapi32
msvc*: DEFINES += _CRT_SECURE_NO_WARNINGS
!win32:VERSION = 1.0.0

include(zlib/zlib.pri)
include(karchive.pri)

TEMPNAME = $${QMAKE_QMAKE}
QTPATH = $$dirname(TEMPNAME)

#OBJECTS_DIR=.obj
#MOC_DIR=.moc

#message($$[QT_LIBS])
unix{
    CONFIG += lib_bundle
    QMAKE_PKGCONFIG_DESTDIR = pkgconfig
    FRAMEWORK_HEADERS.version = Versions
    FRAMEWORK_HEADERS.files = $$KARCHIVE_PUBLIC_HEADERS
    FRAMEWORK_HEADERS.path = Headers
    QMAKE_BUNDLE_DATA += FRAMEWORK_HEADERS
    target.path=$$QTPATH/../lib/$${LIB_ARCH}
    INSTALLS = target
}

android{
    CONFIG -= android_install
    headers.path=$$[QT_INSTALL_HEADERS]/karchive
    headers.files=$$KARCHIVE_PUBLIC_HEADERS
    target.path=$$[QT_HOST_LIBS]
    INSTALLS = headers target
}

win32:*g++* {
    headers.path=$$[QT_INSTALL_HEADERS]/karchive
    headers.files=$$KARCHIVE_PUBLIC_HEADERS

    copy_dll.files = $$[QT_HOST_LIBS]/karchive.dll
    copy_dll.path = $$QTPATH

    target.path=$$[QT_HOST_LIBS]

    INSTALLS = headers target copy_dll
}

linux:!android{
    headers.path=$$[QT_INSTALL_HEADERS]/karchive
    headers.files=$$KARCHIVE_PUBLIC_HEADERS
    target.path=$$[QT_INSTALL_HEADERS]/../lib
    INSTALLS = headers target
}
