#!/bin/bash
# Builds a distributable Svitlo.dmg from a Release build.
#
# Usage:
#   packaging/make-dmg.sh <path/to/Svitlo.app> [path/to/Qt/6.x.y/macos]
#
# Example:
#   packaging/make-dmg.sh build/Qt_6_12_0_for_macOS-Release/Svitlo.app ~/Qt/6.12.0/macos

set -euo pipefail

APP_SRC="${1:?Path to Svitlo.app is required}"
QT_DIR="${2:-$HOME/Qt/6.12.0/macos}"
MACDEPLOYQT="$QT_DIR/bin/macdeployqt"

if [[ ! -d "$APP_SRC" ]]; then
    echo "Not found: $APP_SRC" >&2
    exit 1
fi
if [[ ! -x "$MACDEPLOYQT" ]]; then
    echo "macdeployqt not found at $MACDEPLOYQT — pass your Qt folder as the 2nd argument" >&2
    exit 1
fi

VERSION=$(/usr/libexec/PlistBuddy -c "Print CFBundleShortVersionString" "$APP_SRC/Contents/Info.plist" 2>/dev/null || echo "0.1.0")
ARCH=$(uname -m)
OUT_DIR="$(pwd)/dist"
STAGE="$OUT_DIR/stage"
DMG="$OUT_DIR/Svitlo-$VERSION-$ARCH.dmg"

echo "==> Preparing a clean copy of the app"
rm -rf "$STAGE" "$DMG"
mkdir -p "$STAGE"
ditto "$APP_SRC" "$STAGE/Svitlo.app"
APP="$STAGE/Svitlo.app"

echo "==> Bundling Qt frameworks and libraries (macdeployqt)"
"$MACDEPLOYQT" "$APP" -verbose=1

echo "==> Checking for libraries still pointing outside the app"
LEFT=$( { otool -L "$APP/Contents/MacOS/Svitlo"
          find "$APP/Contents/Frameworks" -name "*.dylib" -exec otool -L {} \; 2>/dev/null; } \
        | grep -E "/opt/homebrew|/usr/local|/opt/local" || true)
if [[ -n "$LEFT" ]]; then
    echo "WARNING: these libraries are not bundled; the app will only run on Macs that have them:" >&2
    echo "$LEFT" >&2
fi

echo "==> Ad-hoc code signing"
codesign --force --deep --sign - "$APP"
codesign --verify --deep --strict "$APP"

echo "==> Creating the disk image"
ln -s /Applications "$STAGE/Applications"
# hdiutil sometimes fails with "Resource busy" (common on CI machines): retry a few times.
for attempt in 1 2 3 4 5; do
    if hdiutil create -volname "Svitlo" -srcfolder "$STAGE" -ov -format UDZO "$DMG" >/dev/null; then
        break
    fi
    if [[ $attempt == 5 ]]; then
        echo "hdiutil failed" >&2
        exit 1
    fi
    echo "hdiutil failed, retrying ($attempt)…" >&2
    sleep 5
done
rm -rf "$STAGE"

echo
echo "Done: $DMG"
