#!/usr/bin/env bash
set -eo pipefail

# Downloads the prebuilt AccessKit C bindings (https://github.com/AccessKit/accesskit-c)
# used by IGraphics when IGRAPHICS_ACCESSKIT is defined.
# The release archive contains headers and libraries for all platforms.
# Result: Dependencies/Build/accesskit/{include,lib}

ACCESSKIT_VERSION=0.23.2

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/../Build"
DEST_DIR="$BUILD_DIR/accesskit"
ZIP_FILE="accesskit-c-$ACCESSKIT_VERSION.zip"
TMP_DIR="$BUILD_DIR/tmp-accesskit"

mkdir -p "$BUILD_DIR"
rm -rf "$TMP_DIR"
mkdir -p "$TMP_DIR"

echo "Downloading AccessKit $ACCESSKIT_VERSION..."
curl -fsSL -o "$TMP_DIR/$ZIP_FILE" \
  "https://github.com/AccessKit/accesskit-c/releases/download/$ACCESSKIT_VERSION/$ZIP_FILE"

unzip -q -o "$TMP_DIR/$ZIP_FILE" -d "$TMP_DIR"

# The archive has a single top level folder
SRC_DIR="$(dirname "$(find "$TMP_DIR" -maxdepth 3 -type d -name include | head -n 1)")"

if [ ! -f "$SRC_DIR/include/accesskit.h" ]; then
  echo "error: unexpected archive layout, accesskit.h not found"
  exit 1
fi

rm -rf "$DEST_DIR"
mv "$SRC_DIR" "$DEST_DIR"
rm -rf "$TMP_DIR"

# The release ships macOS arm64 and x86_64 separately, make a universal static library
if [ "$(uname)" == "Darwin" ]; then
  MAC_LIB_DIR="$DEST_DIR/lib/macos"
  if [ -f "$MAC_LIB_DIR/arm64/static/libaccesskit.a" ] && [ -f "$MAC_LIB_DIR/x86_64/static/libaccesskit.a" ]; then
    mkdir -p "$MAC_LIB_DIR/universal/static"
    lipo -create \
      "$MAC_LIB_DIR/arm64/static/libaccesskit.a" \
      "$MAC_LIB_DIR/x86_64/static/libaccesskit.a" \
      -output "$MAC_LIB_DIR/universal/static/libaccesskit.a"
    echo "Created universal macOS library"
  fi
fi

echo "AccessKit $ACCESSKIT_VERSION installed in $DEST_DIR"
