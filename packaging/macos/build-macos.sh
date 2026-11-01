#!/usr/bin/env bash
# ----------------------------------------------------------------------------
# Cross-build 360WeatherDash.app for macOS (Apple Silicon, arm64) from Linux.
#
#   ./packaging/macos/build-macos.sh
#
# ⚠ LICENSE CAVEAT: this uses Apple's macOS SDK on non-Apple hardware via
# osxcross, which is at odds with the SDK's EULA (Apple-hardware-only). The
# user accepted this explicitly (Nov 2026) for personal builds. Output is
# ad-hoc code-signed (launches locally; Gatekeeper needs right-click → Open
# after download) but NOT notarized — do not distribute this way.
#
# Toolchain: Docker image from packaging/macos/Dockerfile (osxcross clang +
# MacOSX14.4 SDK). Qt = official 6.10.2 macos binaries (universal2 frameworks)
# via aqtinstall; the macOS Qt's Mach-O moc is swapped for the same-version
# Linux moc, so AUTOMOC runs natively (no wine). The app itself needed zero
# source changes.
#
# Output:
#   dist/360WeatherDash-1.0.0-macos-arm64/360WeatherDash.app
#   dist/360WeatherDash-1.0.0-macos-arm64.zip     (~24 MB, symlinks preserved)
#
# The icon (Contents/Resources/icon.icns) is generated from the app's own
# runtime icon renderer (src/AppIcon.h) via a host harness — requires Qt 6
# dev packages on the host (same as a normal Linux build).
#
# Intel Macs: frameworks are universal2, so a x86_64 app is one switch away —
# build inside the container with TRIPLE=x86_64-apple-darwin* and zip as
# -macos-x64. Not wired up here; ask if needed.
#
# Verification from Linux is STATIC only (otool closure, lipo, rpaths,
# LC_CODE_SIGNATURE). There is no legal macOS runtime here — first real run
# happens on actual Apple hardware.
# ----------------------------------------------------------------------------
set -euo pipefail
cd "$(dirname "$0")/../.."

VER=1.0.0
QTVER=6.10.2
IMG=360weatherdash-mac-cross:latest

echo "== icon set (host Qt, uses the app's own AppIcon renderer) =="
ICONS=$(mktemp -d)
cat > "$ICONS/main.cpp" <<'EOF'
#include "AppIcon.h"
#include <QGuiApplication>
int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    for (int px : {16, 32, 128, 256, 512, 1024})
        if (!AppIcon::pixmap(px).save(QString("%1/icon-%2.png").arg(argv[1], QString::number(px))))
            return 1;
    return 0;
}
EOF
cat > "$ICONS/CMakeLists.txt" <<EOF
cmake_minimum_required(VERSION 3.21)
project(iconharness LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
find_package(Qt6 REQUIRED COMPONENTS Gui)
add_executable(iconharness main.cpp)
target_include_directories(iconharness PRIVATE "$PWD/src")
target_link_libraries(iconharness PRIVATE Qt6::Gui)
EOF
cmake -S "$ICONS" -B "$ICONS/b" -GNinja >/dev/null
cmake --build "$ICONS/b" >/dev/null
QT_QPA_PLATFORM=offscreen "$ICONS/b/iconharness" "$ICONS"
python3 - "$ICONS" <<'EOF'
import struct, sys
d = sys.argv[1]
sizes = {16: 'icp4', 32: 'icp5', 128: 'ic07', 256: 'ic08', 512: 'ic09', 1024: 'ic10'}
chunks = b''
for px, code in sizes.items():
    png = open(f'{d}/icon-{px}.png', 'rb').read()
    chunks += code.encode() + struct.pack('>I', len(png) + 8) + png
open(f'{d}/icon.icns', 'wb').write(b'icns' + struct.pack('>I', len(chunks) + 8) + chunks)
EOF
echo "   icon.icns ready"

echo "== toolchain image =="
docker build -q -t "$IMG" -f packaging/macos/Dockerfile packaging/macos/

echo "== cross-build + assemble + verify =="
docker run --rm \
  -v "$PWD:/src:ro" \
  -v "$PWD/dist:/out" \
  -v "$ICONS:/icons:ro" \
  -v 360weatherdash-mac-qt:/opt/Qt \
  "$IMG" bash -euxo pipefail -c '
QTVER='"$QTVER"'
VER='"$VER"'
MACQT=/opt/Qt/$QTVER/macos
LINQT=/opt/Qt/$QTVER/gcc_64
APP=/out/360WeatherDash-$VER-macos-arm64/360WeatherDash.app
TRIPLE=$(ls /opt/osxcross/target/bin | grep -oE "^arm64-apple-darwin[0-9.]+" | head -1)
SDK=/opt/osxcross/target/SDK/MacOSX14.4.sdk

if [ ! -d "$MACQT" ]; then
  pip install --quiet aqtinstall
  aqt install-qt mac   desktop $QTVER clang_64    --archives qtbase -O /opt/Qt
  aqt install-qt linux desktop $QTVER linux_gcc_64 --archives qtbase -O /opt/Qt
fi
# AUTOMOC without wine: swap the macOS Qt Mach-O moc for the Linux one (same version)
pip show aqtinstall >/dev/null 2>&1 || pip install --quiet aqtinstall
for t in moc rcc uic qdbuscpp2xml; do
  lh=$(find "$LINQT" -name "$t" -type f | head -1)
  [ -n "$lh" ] && cp -f "$lh" "$(find "$MACQT" -name "$t" -type f | head -1)" || true
done

cat > /tmp/darwin-toolchain.cmake <<TC
set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_PROCESSOR arm64)
set(CMAKE_OSX_ARCHITECTURES arm64)
set(CMAKE_OSX_DEPLOYMENT_TARGET 13.0)
set(CMAKE_C_COMPILER   /opt/osxcross/target/bin/$TRIPLE-clang)
set(CMAKE_CXX_COMPILER /opt/osxcross/target/bin/$TRIPLE-clang++)
set(CMAKE_FIND_ROOT_PATH "$MACQT" $SDK)
set(CMAKE_OSX_SYSROOT $SDK)
set(CMAKE_FRAMEWORK_PATH $SDK/System/Library/Frameworks)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
set(CMAKE_SKIP_RPATH ON)
TC

# rpath must come from the LINK line. Editing a signed arm64 binary with
# install_name_tool invalidates the ad-hoc code signature (dyld kill; macOS
# then reports the app as damaged or corrupt).
cmake -S /src -B /tmp/b -GNinja -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=/tmp/darwin-toolchain.cmake -DCMAKE_PREFIX_PATH="$MACQT" \
      "-DCMAKE_EXE_LINKER_FLAGS=-Wl,-rpath,@executable_path/../Frameworks"
cmake --build /tmp/b
llvm-objdump -f /tmp/b/360WeatherDash 2>/dev/null | grep "file format" || true

rm -rf "$(dirname "$APP")"
mkdir -p "$APP"/Contents/{MacOS,Resources,Frameworks,PlugIns}
cp /tmp/b/360WeatherDash "$APP/Contents/MacOS/"
cp /icons/icon.icns "$APP/Contents/Resources/"
# QtDBus is required by QtGui on macOS — do not prune it
for fw in QtCore QtGui QtNetwork QtDBus QtWidgets; do
  cp -R "$MACQT/lib/$fw.framework" "$APP/Contents/Frameworks/"
done
for d in platforms styles tls networkinformation; do
  [ -d "$MACQT/plugins/$d" ] && mkdir -p "$APP/Contents/PlugIns/$d" \
    && cp "$MACQT/plugins/$d/"*.dylib "$APP/Contents/PlugIns/$d/"
done
cat > "$APP/Contents/Info.plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleName</key><string>360WeatherDash</string>
  <key>CFBundleDisplayName</key><string>360WeatherDash</string>
  <key>CFBundleIdentifier</key><string>com.360weatherdash.app</string>
  <key>CFBundleVersion</key><string>'"$VER"'</string>
  <key>CFBundleShortVersionString</key><string>'"$VER"'</string>
  <key>CFBundlePackageType</key><string>APPL</string>
  <key>CFBundleExecutable</key><string>360WeatherDash</string>
  <key>CFBundleIconFile</key><string>icon</string>
  <key>LSMinimumSystemVersion</key><string>13.0</string>
  <key>NSHighResolutionCapable</key><true/>
  <key>NSSupportsAutomaticGraphicsSwitching</key><true/>
</dict>
</plist>
PLIST

O=/opt/osxcross/target/bin
OT=$(ls $O | grep -E "^arm64.*-otool$" | head -1)

echo "== static verification =="
# rpath must be exactly right from the link line — no post-link edits allowed
RPATHS=$($O/$OT -l "$APP/Contents/MacOS/360WeatherDash" | grep -A2 LC_RPATH | grep path | awk "{print \$2}")
echo "rpaths: $RPATHS"
echo "$RPATHS" | grep -qx "@executable_path/../Frameworks" || { echo "FATAL: bundle rpath missing"; exit 1; }
echo "$RPATHS" | grep -q "/opt/Qt" && { echo "FATAL: build-tree rpath leaked"; exit 1; }
$O/$(ls $O | grep -E "^arm64.*-lipo$" | head -1) -info "$APP/Contents/MacOS/360WeatherDash"
$O/$OT -l "$APP/Contents/MacOS/360WeatherDash" | grep -A2 LC_RPATH | grep path
$O/$OT -l "$APP/Contents/MacOS/360WeatherDash" | grep -c LC_CODE_SIGNATURE | sed "s/^/LC_CODE_SIGNATURE count: /"
miss=0
check_one() {
  $O/$OT -L "$1" 2>/dev/null | grep -v "(architecture .*):$" | grep -v ":$" |
  sed "s/^\s*//" | awk "{print \$1}" | while read lib; do
    case "$lib" in
      /usr/lib/*|/System/*) ;;
      @rpath/*) rel=${lib#@rpath/}
        [ -e "$APP/Contents/Frameworks/$rel" ] || { echo "MISSING: $lib (in $1)"; exit 1; } ;;
      *) echo "ODD: $lib (in $1)"; exit 1 ;;
    esac
  done || miss=1
}
check_one "$APP/Contents/MacOS/360WeatherDash"
for fw in "$APP"/Contents/Frameworks/*.framework; do check_one "$fw/Versions/A/$(basename "$fw" .framework)"; done
for p in "$APP"/Contents/PlugIns/*/*.dylib; do check_one "$p"; done
[ $miss = 0 ] && echo "CLOSURE OK"
[ -n "$(find "$APP" -type l)" ] && echo "symlinks present: $(find "$APP" -type l | wc -l)"

# zip -y: framework symlink layout is sealed by the ad-hoc signatures —
# flattening it (default zip behavior) breaks the seals and dyld kills the app
cd "$(dirname "$APP")"
rm -f /out/360WeatherDash-$VER-macos-arm64.zip
zip -qry /out/360WeatherDash-$VER-macos-arm64.zip 360WeatherDash.app
du -sh /out/360WeatherDash-$VER-macos-arm64{,.zip}
'
echo "OK: dist/360WeatherDash-$VER-macos-arm64.zip — unzip, then right-click 360WeatherDash.app → Open (unsigned/ad-hoc)"
