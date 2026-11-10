#!/usr/bin/env bash
# Collects CI build artifacts (downloaded into $1) into a flat directory ($2)
# ready to be attached to a GitHub release.
set -euo pipefail

src=${1:?artifacts dir}
out=${2:?output dir}
version=$(cat "$src/klogg_version/klogg_version.txt")

mkdir -p "$out"
out=$(cd "$out" && pwd)
shopt -s nullglob

# Windows: installer, portable zip, pdb zip
for f in "$src"/packages-windows-*/*; do
  cp "$f" "$out/"
done

# Linux: deb packages and debug symbols
symbols_dir=$(mktemp -d)
for dir in "$src"/packages-noble "$src"/packages-resolute; do
  [ -d "$dir" ] || continue
  for f in "$dir"/*.deb; do cp "$f" "$out/"; done
  for f in "$dir"/*.debug.xz; do cp "$f" "$symbols_dir/"; done
done
symbols=("$symbols_dir"/*)
if [ ${#symbols[@]} -gt 0 ]; then
  tar -C "$symbols_dir" -cf "$out/klogg-$version-linux-symbols.tar" .
fi
rm -rf "$symbols_dir"

# Linux: plain binary tarball extracted from the Ubuntu 24.04 package
for f in "$src"/packages-noble/*.deb; do
  dpkg-deb --fsys-tarfile "$f" | gzip -9 > "$out/klogg-$version-linux-bin.tar.gz"
  break
done

# macOS: dmg and zipped dSYM bundle (the .app is already inside the dmg)
for dir in "$src"/packages-macos-*; do
  for f in "$dir"/*.dmg; do cp "$f" "$out/"; done
  for d in "$dir"/*.dSym; do
    name=$(basename "$d" .dSym)
    (cd "$(dirname "$d")" && zip -qr "$out/klogg-$version-mac-${name#klogg-}-dSYM.zip" "$(basename "$d")")
  done
done

cp "$src/klogg_version/klogg_version.txt" "$out/"

(cd "$out" && sha256sum --binary -- * > "klogg-$version-sha256.txt")

echo "Release assets:"
ls -la "$out"
