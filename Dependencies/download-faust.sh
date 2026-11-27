#!/usr/bin/env bash
# Downloads a prebuilt libfaust from https://github.com/iPlug2/faust-builder into Dependencies/Build/faust, where the
# iPlug2 CMake Faust module (Scripts/cmake/Faust.cmake) finds it:
#
#   Build/faust/include/faust         Faust architecture headers
#   Build/faust/share/faust           Faust standard libraries (stdfaust.lib etc.), matching the libfaust version
# Apple (macOS, iOS, visionOS):
#   Build/faust/LibFaust.xcframework  static libfaust (interpreter backend)
# Windows (MSVC, static runtime):
#   Build/faust/lib/<arch>/libfaust.lib, libfaustd.lib   static libfaust for x64, ARM64 and ARM64EC (libfaustd.lib: /MTd, for Debug)
#   Build/faust/bin/faust.exe                            the faust compiler (x64), to generate C++ for release builds
#
# Usage:
#   ./download-faust.sh [--platform apple|windows]                    download the pinned release (FAUST_BUILDER_VERSION below), verifying its checksums
#   ./download-faust.sh [--platform apple|windows] v2.96.0            download another release (not verified, there are no pinned checksums for it)
#   ./download-faust.sh [--platform apple|windows] --local <dir>      copy from a local faust-builder build folder (e.g. ~/faust-builder/build)
#
# The platform defaults to the one the script runs on (windows in Git Bash/MSYS2, otherwise apple).
# On Apple platforms the faust executable is not included, install Faust for that (e.g. brew install faust).

set -eo pipefail

FAUST_BUILDER_VERSION="v2.95.3.1"

# SHA256 of the pinned release's assets (they match the digests GitHub shows for the release)
SHA256_XCFRAMEWORK="bc186c5f6fa8208eb50181c3338c007bd09bc720241ee63320aee8eff8353129"
SHA256_WINDOWS="af62eb89026f4add73be203d459d2cf45c58e8fed168e232e56e24b871cc9c12"
SHA256_HEADERS="50138124b4df606c3183fa12557a47802a7aeb8f25b1974f44bd469d0db0fde2"
SHA256_LIBRARIES="207be78ed7d25510e76f66848df7e09769fa82eef206a16452ee49502b5fc551"

cd "$(dirname "$0")"
DEST="$PWD/Build/faust"
REPO_URL="https://github.com/iPlug2/faust-builder/releases"

usage() {
  echo "usage: $0 [--platform apple|windows] [version | --local <faust-builder build dir>]"
  exit 1
}

case "$(uname -s)" in
  MINGW*|MSYS*|CYGWIN*) PLATFORM="windows" ;;
  *) PLATFORM="apple" ;;
esac

if [ "$1" == "--platform" ]; then
  PLATFORM="$2"
  shift 2 || usage
fi
[ "$PLATFORM" == "apple" ] || [ "$PLATFORM" == "windows" ] || usage

# Everything is assembled in a temporary folder, which replaces Build/faust only when it is complete
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
STAGE="$TMP/faust"
mkdir -p "$STAGE/include" "$STAGE/share"

sha256() {
  if command -v sha256sum > /dev/null; then
    sha256sum "$1" | cut -d ' ' -f 1
  else
    shasum -a 256 "$1" | cut -d ' ' -f 1
  fi
}

verify() { # file expected-sha256
  local actual
  actual="$(sha256 "$1")"
  if [ "$actual" != "$2" ]; then
    echo "error: checksum mismatch for $(basename "$1") (expected $2, got $actual)"
    exit 1
  fi
}

if [ "$1" == "--local" ]; then
  [ -n "$2" ] || usage
  SRC="${2%/}"

  if [ "$PLATFORM" == "windows" ]; then
    if [ ! -d "$SRC/windows/lib" ] || [ ! -d "$SRC/headers/faust" ]; then
      echo "error: $SRC doesn't look like a faust-builder Windows build folder (expected windows/lib and headers/faust)"
      exit 1
    fi
    cp -R "$SRC/windows/lib" "$STAGE/"
    if [ -d "$SRC/windows/bin" ]; then
      cp -R "$SRC/windows/bin" "$STAGE/"
    fi
  else
    if [ ! -d "$SRC/LibFaust.xcframework" ] || [ ! -d "$SRC/headers/faust" ]; then
      echo "error: $SRC doesn't look like a faust-builder build folder (expected LibFaust.xcframework and headers/faust)"
      exit 1
    fi
    cp -R "$SRC/LibFaust.xcframework" "$STAGE/"
  fi

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

  if [ "$PLATFORM" == "windows" ]; then
    LIB_ASSET="libfaust-windows.tar.gz"
    SHA256_LIB="$SHA256_WINDOWS"
  else
    LIB_ASSET="LibFaust.xcframework.zip"
    SHA256_LIB="$SHA256_XCFRAMEWORK"
  fi

  echo "Downloading libfaust $VERSION for $PLATFORM from $REPO_URL"
  if ! curl -fsSL --retry 3 --retry-all-errors --retry-delay 2 -o "$TMP/$LIB_ASSET" "$BASE_URL/$LIB_ASSET"; then
    echo "error: downloading $LIB_ASSET failed (does release $VERSION have it?)"
    exit 1
  fi
  curl -fsSL --retry 3 --retry-all-errors --retry-delay 2 -o "$TMP/libfaust-headers.tar.gz" "$BASE_URL/libfaust-headers.tar.gz"

  # Older releases have no libraries: only a 404 means that, any other failure is an error
  HTTP_STATUS="$(curl -sSL --retry 3 --retry-all-errors --retry-delay 2 -o "$TMP/faust-libraries.tar.gz" -w "%{http_code}" "$BASE_URL/faust-libraries.tar.gz")"
  if [ "$HTTP_STATUS" == "404" ]; then
    echo "warning: $VERSION has no faust-libraries.tar.gz, the Faust libraries were not downloaded"
    rm -f "$TMP/faust-libraries.tar.gz"
  elif [ "$HTTP_STATUS" != "200" ]; then
    echo "error: downloading faust-libraries.tar.gz failed (HTTP $HTTP_STATUS)"
    exit 1
  fi

  if [ "$VERSION" == "$FAUST_BUILDER_VERSION" ] && [ -n "$SHA256_LIB" ]; then
    verify "$TMP/$LIB_ASSET" "$SHA256_LIB"
    verify "$TMP/libfaust-headers.tar.gz" "$SHA256_HEADERS"
    verify "$TMP/faust-libraries.tar.gz" "$SHA256_LIBRARIES"
    echo "Checksums verified"
  else
    echo "warning: no pinned checksums for $VERSION ($PLATFORM), the download was not verified"
  fi

  if [ "$PLATFORM" == "windows" ]; then
    # Extracts to windows/lib/<arch> and windows/bin
    tar -xzf "$TMP/$LIB_ASSET" -C "$TMP"
    cp -R "$TMP/windows/lib" "$STAGE/"
    cp -R "$TMP/windows/bin" "$STAGE/"
  else
    unzip -q "$TMP/$LIB_ASSET" -d "$STAGE"
  fi

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
