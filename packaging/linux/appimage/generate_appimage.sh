#!/bin/bash

DESTDIR=$(readlink -f appdir) ninja install
wget -c -q "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
chmod a+x linuxdeployqt-continuous-x86_64.AppImage
VERSION=$KLOGG_VERSION ./linuxdeployqt-continuous-x86_64.AppImage appdir/usr/share/applications/*.desktop -bundle-non-qt-libs
VERSION=$KLOGG_VERSION ./linuxdeployqt-continuous-x86_64.AppImage appdir/usr/share/applications/*.desktop -appimage
mkdir ./packages
cp ./klogg-$KLOGG_VERSION-x86_64.AppImage ./packages/klogg-$KLOGG_VERSION-x86_64.AppImage
