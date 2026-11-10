#!/usr/bin/env bash
# Builds klogg AppImage with linuxdeploy + linuxdeploy-plugin-qt.
# Run from the CMake build directory after the build has finished.
# Uses qmake from QMAKE env var or PATH to locate Qt.
set -euo pipefail

LINUXDEPLOY_VERSION=1-alpha-20251107-1
LINUXDEPLOY_QT_VERSION=1-alpha-20250213-1

version=${KLOGG_VERSION:-dev}
build_root=$(pwd)
appdir="$build_root/AppDir"
tools="$build_root/appimage-tools"

rm -rf "$appdir"
DESTDIR="$appdir" cmake --install "$build_root" --prefix /usr

mkdir -p "$tools"
download() {
  [ -x "$tools/$2" ] || {
    curl --fail --silent --show-error --location --output "$tools/$2" "$1"
    chmod +x "$tools/$2"
  }
}
download "https://github.com/linuxdeploy/linuxdeploy/releases/download/$LINUXDEPLOY_VERSION/linuxdeploy-x86_64.AppImage" linuxdeploy-x86_64.AppImage
download "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/$LINUXDEPLOY_QT_VERSION/linuxdeploy-plugin-qt-x86_64.AppImage" linuxdeploy-plugin-qt-x86_64.AppImage

# no FUSE in containers/CI runners
export APPIMAGE_EXTRACT_AND_RUN=1
export PATH="$tools:$PATH"
export QMAKE=${QMAKE:-$(command -v qmake6 || command -v qmake)}
# TLS backends for https (version check), wayland support
export EXTRA_QT_MODULES="network;waylandclient"
export EXTRA_PLATFORM_PLUGINS="libqwayland-generic.so"
export LDAI_OUTPUT="klogg-$version-x86_64.AppImage"
export LINUXDEPLOY_OUTPUT_VERSION="$version"

linuxdeploy-x86_64.AppImage \
  --appdir "$appdir" \
  --executable "$appdir/usr/bin/klogg" \
  --desktop-file "$appdir/usr/share/applications/klogg.desktop" \
  --icon-file "$appdir/usr/share/icons/hicolor/scalable/apps/klogg.svg" \
  --plugin qt \
  --output appimage

mkdir -p "$build_root/packages"
mv "$LDAI_OUTPUT" "$build_root/packages/"
echo "Created $build_root/packages/$LDAI_OUTPUT"
