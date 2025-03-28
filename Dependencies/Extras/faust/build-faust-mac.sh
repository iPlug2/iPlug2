#!/usr/bin/env bash
set -e

#should be executed in Extras/faust
DEPS_DIR="$PWD"
BUILD_DIR="$PWD/../../Build"
SRC_DIR="$BUILD_DIR/src"
TMP_DIR="$BUILD_DIR/tmp"
FAUST_REPO_DIR="$TMP_DIR/faust"
FAUST_CMAKE_BUILD_DIR="$TMP_DIR/faust-cmake"
INSTALL_DIR="$BUILD_DIR/mac"

if [ -d "$FAUST_REPO_DIR" ]
then
  echo faust repo exists
else
  git clone --recursive https://github.com/grame-cncm/faust.git $FAUST_REPO_DIR
fi
mkdir $FAUST_CMAKE_BUILD_DIR
cd $FAUST_CMAKE_BUILD_DIR
cmake -C $DEPS_DIR/iplug-backends.cmake -C $DEPS_DIR/iplug-targets-mac.cmake -DINCLUDE_STATIC=on -DINCLUDE_DYNAMIC=on -DINCLUDE_OSC=off -DINCLUDE_HTTP=off -DUNIVERSAL=off ../faust/build
cmake -D CMAKE_INSTALL_PREFIX=$INSTALL_DIR ../faust/build
cmake --build $FAUST_CMAKE_BUILD_DIR --config Release -- -j 6
make install
rm $INSTALL_DIR/lib/ios-libsndfile.a
rm $INSTALL_DIR/lib/libfaust.dylib
rm $INSTALL_DIR/lib/libfaust.2.dylib
mv $INSTALL_DIR/lib/*.dylib $INSTALL_DIR/lib/libfaust.dylib
install_name_tool -id @loader_path/../Frameworks/libfaust.dylib $INSTALL_DIR/lib/libfaust.dylib
