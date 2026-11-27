#!/usr/bin/env bash
# Downloads a prebuilt libfaust for Apple platforms (macOS, iOS, visionOS) from https://github.com/iPlug2/faust-builder
# into Dependencies/Build/faust, where the iPlug2 CMake Faust module (Scripts/cmake/Faust.cmake) finds it:
#
#   Build/faust/LibFaust.xcframework  static libfaust (interpreter + C++ backends)
#   Build/faust/include/faust         Faust architecture headers
#   Build/faust/share/faust           Faust standard libraries (stdfaust.lib etc.), matching the libfaust version
#
# Usage:
#   ./download-faust.sh                      download the latest release
#   ./download-faust.sh v2.95.3              download a specific release
#   ./download-faust.sh --local <dir>        copy from a local faust-builder build folder (e.g. ~/faust-builder/build)
#
# The faust executable (used to generate C++ for release builds) is not included, install Faust for that.

set -eo pipefail

cd "$(dirname "$0")"
DEST="$PWD/Build/faust"
REPO_URL="https://github.com/iPlug2/faust-builder/releases"

if [ "$1" == "--local" ]; then
  SRC="${2%/}"
  if [ ! -d "$SRC/LibFaust.xcframework" ] || [ ! -d "$SRC/headers/faust" ]; then
    echo "error: $SRC doesn't look like a faust-builder build folder (expected LibFaust.xcframework and headers/faust)"
    exit 1
  fi

  rm -rf "$DEST"
  mkdir -p "$DEST/include" "$DEST/share"
  cp -R "$SRC/LibFaust.xcframework" "$DEST/"
  cp -R "$SRC/headers/faust" "$DEST/include/"

  if [ -d "$SRC/libraries/faust" ]; then
    cp -R "$SRC/libraries/faust" "$DEST/share/"
  else
    echo "warning: $SRC/libraries/faust not found, the Faust libraries were not copied"
  fi
else
  VERSION="${1:-latest}"

  if [ "$VERSION" == "latest" ]; then
    BASE_URL="$REPO_URL/latest/download"
  else
    BASE_URL="$REPO_URL/download/$VERSION"
  fi

  TMP="$(mktemp -d)"
  trap 'rm -rf "$TMP"' EXIT

  echo "Downloading libfaust ($VERSION) from $REPO_URL"
  curl -fL --progress-bar -o "$TMP/LibFaust.xcframework.zip" "$BASE_URL/LibFaust.xcframework.zip"
  curl -fL --progress-bar -o "$TMP/libfaust-headers.tar.gz" "$BASE_URL/libfaust-headers.tar.gz"

  rm -rf "$DEST"
  mkdir -p "$DEST/include" "$DEST/share"
  unzip -q "$TMP/LibFaust.xcframework.zip" -d "$DEST"
  tar -xzf "$TMP/libfaust-headers.tar.gz" -C "$TMP"
  cp -R "$TMP/headers/faust" "$DEST/include/"

  if curl -fsL -o "$TMP/faust-libraries.tar.gz" "$BASE_URL/faust-libraries.tar.gz"; then
    tar -xzf "$TMP/faust-libraries.tar.gz" -C "$DEST/share"
  else
    echo "warning: this release has no faust-libraries.tar.gz, the Faust libraries were not downloaded"
  fi
fi

echo "libfaust installed in $DEST"
grep -h "define FAUSTVERSION" "$DEST/include/faust/export.h" 2>/dev/null || true
