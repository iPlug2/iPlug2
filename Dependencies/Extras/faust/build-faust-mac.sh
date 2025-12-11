#!/usr/bin/env bash
set -e

#should be executed in Extras/faust
DEPS_DIR="$PWD"
BUILD_DIR="$PWD/../../Build"
SRC_DIR="$BUILD_DIR/src"
TMP_DIR="$BUILD_DIR/tmp"
FAUST_REPO_DIR="$TMP_DIR/faust"
FAUST_CMAKE_BUILD_DIR="$FAUST_REPO_DIR/build/faustdir"
INSTALL_DIR="$BUILD_DIR/mac"

if [ -d "$FAUST_REPO_DIR" ]
then
  echo "faust repo exists"
else
  git clone --recursive https://github.com/grame-cncm/faust.git "$FAUST_REPO_DIR"
fi

# Build using FAUST's Makefile wrapper with iPlug2 config files
cd "$FAUST_REPO_DIR"

# Copy iPlug2 backend/target configs to FAUST's config directories
cp "$DEPS_DIR/iplug-backends.cmake" build/backends/iplug.cmake
cp "$DEPS_DIR/iplug-targets-mac.cmake" build/targets/iplug.cmake

# Configure and build static library with interpreter backend
cd build
make cmake BACKENDS=iplug.cmake TARGETS=iplug.cmake
make staticlib -j6
cd ..

# Copy built library to install location
mkdir -p "$INSTALL_DIR/lib"
mkdir -p "$INSTALL_DIR/include/faust/dsp"
mkdir -p "$INSTALL_DIR/include/faust/gui"

cp "$FAUST_REPO_DIR/build/lib/libfaust.a" "$INSTALL_DIR/lib/"
cp "$FAUST_REPO_DIR/architecture/faust/"*.h "$INSTALL_DIR/include/faust/"
cp -r "$FAUST_REPO_DIR/architecture/faust/dsp/"* "$INSTALL_DIR/include/faust/dsp/"
cp -r "$FAUST_REPO_DIR/architecture/faust/gui/"* "$INSTALL_DIR/include/faust/gui/"

echo "FAUST built successfully to $INSTALL_DIR"
