#!/bin/bash

DESTDIR=$(readlink -f appdir) ninja install
wget -c https://github.com/$(wget -q https://github.com/probonopd/go-appimage/releases/expanded_assets/continuous -O - | grep "appimagetool-.*-x86_64.AppImage" | head -n 1 | cut -d '"' -f 2)
chmod +x appimagetool-*.AppImage
VERSION=$KLOGG_VERSION ./appimagetool-*.AppImage -s deploy appdir/usr/share/applications/*.desktop # Bundle EVERYTHING
mkdir ./packages
cp ./klogg-$KLOGG_VERSION-x86_64.AppImage ./packages/klogg-$KLOGG_VERSION-x86_64.AppImage
