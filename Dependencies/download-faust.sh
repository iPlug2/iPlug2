#!/usr/bin/env bash
# Downloads a prebuilt libfaust for Apple platforms (macOS, iOS, visionOS) from https://github.com/iPlug2/faust-builder
# into Dependencies/Build/faust, where the iPlug2 CMake Faust module (Scripts/cmake/Faust.cmake) finds it:
#
#   Build/faust/LibFaust.xcframework  static libfaust (interpreter + C++ backends)
#   Build/faust/include/faust         Faust architecture headers
#   Build/faust/share/faust           Faust standard libraries (stdfaust.lib etc.), matching the libfaust version
#
# Usage:
#   ./download-faust.sh                      download the pinned release (FAUST_BUILDER_VERSION below), verifying its checksums
#   ./download-faust.sh v2.96.0              download another release (not verified, there are no pinned checksums for it)
#   ./download-faust.sh --local <dir>        copy from a local faust-builder build folder (e.g. ~/faust-builder/build)
#
# The faust executable (used to generate C++ for release builds) is not included, install Faust for that.

set -eo pipefail

FAUST_BUILDER_VERSION="v2.95.3"

# SHA256 of the pinned release's assets (they match the digests GitHub shows for the release)
SHA256_XCFRAMEWORK="114a92220f97aea0b51b9b1ebffc15f66bb6d50a8c9ac21aed7934ae73d5bde5"
SHA256_HEADERS="68e6ccd6f4427dfbf88b71b05c16d851d5fc6bdd6ee7b5673cf7d56f3e8337a8"
SHA256_LIBRARIES="682fb1ddcb50720d9f961fbd3799075b2ce4a29441d215d89b0532f01216e022"

cd "$(dirname "$0")"
DEST="$PWD/Build/faust"
REPO_URL="https://github.com/iPlug2/faust-builder/releases"

usage() {
  echo "usage: $0 [version | --local <faust-builder build dir>]"
  exit 1
}

# Everything is assembled in a temporary folder, which replaces Build/faust only when it is complete
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
STAGE="$TMP/faust"
mkdir -p "$STAGE/include" "$STAGE/share"

verify() { # file expected-sha256
  local actual
  actual="$(shasum -a 256 "$1" | cut -d ' ' -f 1)"
  if [ "$actual" != "$2" ]; then
    echo "error: checksum mismatch for $(basename "$1") (expected $2, got $actual)"
    exit 1
  fi
}

if [ "$1" == "--local" ]; then
  [ -n "$2" ] || usage
  SRC="${2%/}"
  if [ ! -d "$SRC/LibFaust.xcframework" ] || [ ! -d "$SRC/headers/faust" ]; then
    echo "error: $SRC doesn't look like a faust-builder build folder (expected LibFaust.xcframework and headers/faust)"
    exit 1
  fi

  cp -R "$SRC/LibFaust.xcframework" "$STAGE/"
  cp -R "$SRC/headers/faust" "$STAGE/include/"

  if [ -d "$SRC/libraries/faust" ]; then
    cp -R "$SRC/libraries/faust" "$STAGE/share/"
  else
    echo "warning: $SRC/libraries/faust not found, the Faust libraries were not copied"
  fi
else
  [ "$#" -le 1 ] || usage
  VERSION="${1:-$FAUST_BUILDER_VERSION}"
  BASE_URL="$REPO_URL/download/$VERSION"

  echo "Downloading libfaust $VERSION from $REPO_URL"
  curl -fsSL -o "$TMP/LibFaust.xcframework.zip" "$BASE_URL/LibFaust.xcframework.zip"
  curl -fsSL -o "$TMP/libfaust-headers.tar.gz" "$BASE_URL/libfaust-headers.tar.gz"

  # Older releases have no libraries: only a 404 means that, any other failure is an error
  HTTP_STATUS="$(curl -sSL -o "$TMP/faust-libraries.tar.gz" -w "%{http_code}" "$BASE_URL/faust-libraries.tar.gz")"
  if [ "$HTTP_STATUS" == "404" ]; then
    echo "warning: $VERSION has no faust-libraries.tar.gz, the Faust libraries were not downloaded"
    rm -f "$TMP/faust-libraries.tar.gz"
  elif [ "$HTTP_STATUS" != "200" ]; then
    echo "error: downloading faust-libraries.tar.gz failed (HTTP $HTTP_STATUS)"
    exit 1
  fi

  if [ "$VERSION" == "$FAUST_BUILDER_VERSION" ]; then
    verify "$TMP/LibFaust.xcframework.zip" "$SHA256_XCFRAMEWORK"
    verify "$TMP/libfaust-headers.tar.gz" "$SHA256_HEADERS"
    verify "$TMP/faust-libraries.tar.gz" "$SHA256_LIBRARIES"
    echo "Checksums verified"
  else
    echo "warning: no pinned checksums for $VERSION, the download was not verified"
  fi

  unzip -q "$TMP/LibFaust.xcframework.zip" -d "$STAGE"
  tar -xzf "$TMP/libfaust-headers.tar.gz" -C "$TMP"
  cp -R "$TMP/headers/faust" "$STAGE/include/"

  if [ -f "$TMP/faust-libraries.tar.gz" ]; then
    tar -xzf "$TMP/faust-libraries.tar.gz" -C "$STAGE/share"
  fi
fi

mkdir -p "$(dirname "$DEST")"
rm -rf "$DEST"
mv "$STAGE" "$DEST"

echo "libfaust installed in $DEST"
grep -h "define FAUSTVERSION" "$DEST/include/faust/export.h" 2>/dev/null || true
