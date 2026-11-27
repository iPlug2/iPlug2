#!/usr/bin/env bash
# Downloads a prebuilt libfaust from https://github.com/iPlug2/faust-builder into Dependencies/Build/faust, where the
# iPlug2 CMake Faust module (Scripts/cmake/Faust.cmake) finds it:
#
#   Build/faust/include/faust         Faust architecture headers
#   Build/faust/share/faust           Faust standard libraries (stdfaust.lib etc.), matching the libfaust version
# Apple (macOS, iOS, visionOS):
#   Build/faust/LibFaust.xcframework  static libfaust (interpreter backend)
# Linux:
#   Build/faust/lib/<arch>/libfaust.a                    static libfaust (interpreter backend) for x86_64 and aarch64
#   Build/faust/bin/faust                                the faust compiler, for the architecture the script runs on
# Windows (MSVC, static runtime):
#   Build/faust/lib/<arch>/libfaust.lib, libfaustd.lib   static libfaust for x64, ARM64 and ARM64EC (libfaustd.lib: /MTd, for Debug)
#   Build/faust/bin/faust.exe                            the faust compiler (x64), to generate C++ for release builds
#
# Usage:
#   ./download-faust.sh [--platform apple|windows|linux]                    download the pinned release (FAUST_BUILDER_VERSION below), verifying its checksums
#   ./download-faust.sh [--platform apple|windows|linux] v2.96.0            download another release (not verified, there are no pinned checksums for it)
#   ./download-faust.sh [--platform apple|windows|linux] --local <dir>      copy from a local faust-builder build folder (e.g. ~/faust-builder/build)
#
# The platform defaults to the one the script runs on (windows in Git Bash/MSYS2, linux, otherwise apple).
# On Apple platforms the faust executable is not included, install Faust for that (e.g. brew install faust).

set -eo pipefail

FAUST_BUILDER_VERSION="v2.95.3.2"

# SHA256 of the pinned release's assets (they match the digests GitHub shows for the release)
SHA256_XCFRAMEWORK="d0edfee5f0ba53be52c4ee0e70178040908a8dae5f482f7145e689290a075a6c"
SHA256_WINDOWS="420c5fb4fdc8bed0bce8bd43d159ad96449020337b019382223c43694149e6c2"
SHA256_LINUX="df2a42daacc297191d0932525c2dacbf8c99cbeecac1e9caa50424c0b1e49302"
SHA256_HEADERS="23da97f50e154cb69ff8df275b70e4b3d80c55a20eccb8ba206a688980c265b8"
SHA256_LIBRARIES="154bc7f21f5a7b3114d403a561e884ce73fa0d6cb5a5796bacb87b7ac305488c"

cd "$(dirname "$0")"
DEST="$PWD/Build/faust"
REPO_URL="https://github.com/iPlug2/faust-builder/releases"

usage() {
  echo "usage: $0 [--platform apple|windows|linux] [version | --local <faust-builder build dir>]"
  exit 1
}

case "$(uname -s)" in
  MINGW*|MSYS*|CYGWIN*) PLATFORM="windows" ;;
  Linux*) PLATFORM="linux" ;;
  *) PLATFORM="apple" ;;
esac

if [ "$1" == "--platform" ]; then
  PLATFORM="$2"
  shift 2 || usage
fi
[ "$PLATFORM" == "apple" ] || [ "$PLATFORM" == "windows" ] || [ "$PLATFORM" == "linux" ] || usage

# Everything is assembled in a temporary folder, which replaces Build/faust only when it is complete
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
STAGE="$TMP/faust"
mkdir -p "$STAGE/include" "$STAGE/share"

# Copies <dir>/lib (libfaust.a for each architecture) and the faust executable for this machine's architecture
install_linux() { # dir
  local arch
  arch="$(uname -m)"
  cp -R "$1/lib" "$STAGE/"
  if [ -f "$1/bin/$arch/faust" ]; then
    mkdir -p "$STAGE/bin"
    cp "$1/bin/$arch/faust" "$STAGE/bin/faust"
  else
    echo "warning: no faust executable for $arch in $1/bin"
  fi
}

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
  elif [ "$PLATFORM" == "linux" ]; then
    if [ ! -d "$SRC/linux/lib" ] || [ ! -d "$SRC/headers/faust" ]; then
      echo "error: $SRC doesn't look like a faust-builder Linux build folder (expected linux/lib and headers/faust)"
      exit 1
    fi
    install_linux "$SRC/linux"
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
  elif [ "$PLATFORM" == "linux" ]; then
    LIB_ASSET="libfaust-linux.tar.gz"
    SHA256_LIB="$SHA256_LINUX"
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
  elif [ "$PLATFORM" == "linux" ]; then
    # Extracts to linux/lib/<arch> and linux/bin/<arch>
    tar -xzf "$TMP/$LIB_ASSET" -C "$TMP"
    install_linux "$TMP/linux"
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
