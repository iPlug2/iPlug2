#!/usr/bin/env bash
set -e

IGRAPHICS_DEPS_DIR="$PWD"
BUILD_DIR="$IGRAPHICS_DEPS_DIR/../Build"
DL_DIR="$BUILD_DIR/tmp"
SRC_DIR="$BUILD_DIR/src"
LOG_PATH="$BUILD_DIR"
LOG_NAME="download.log"

# Basename part of tarballs to download
FREETYPE_VERSION=freetype-2.13.3
PKGCONFIG_VERSION=pkg-config-0.28
EXPAT_VERSION=expat-2.2.5
PNG_VERSION=v1.6.35
ZLIB_VERSION=zlib-1.3.1
SKIA_VERSION=chrome/m130
#SKIA_VERSION=main

# URLs where tarballs of releases can be downloaded - no trailing slash
PNG_URL=https://github.com/glennrp/libpng/archive
ZLIB_URL=https://www.zlib.net
FREETYPE_URL=https://download.savannah.gnu.org/releases/freetype
SKIA_URL=https://github.com/google/skia.git

echo "IGRAPHICS_DEPS_DIR:" $IGRAPHICS_DEPS_DIR
echo "BUILD_DIR:" $BUILD_DIR
echo "DL_DIR:" $DL_DIR
echo "LOG_PATH:" $LOG_PATH
echo "LOG_NAME:" $LOG_NAME

[[ -e "$PWD/download-igraphics-libs.sh" ]] ||
{
  echo "*******************************************************************************"
  echo "Error: Please cd into the folder containing this script before running it.";
  echo "*******************************************************************************"
  exit 1;
}

err_report() {
    echo
    echo "*******************************************************************************"
    echo "Error: something went wrong during the download process, printing $LOG_NAME "
    echo "*******************************************************************************"
    echo
    cat "$LOG_PATH/$LOG_NAME"
}

trap err_report ERR

spin() {
    pid=$! # Process Id of the previous running command
    spin='-\|/'
    i=0
    while kill -0 $pid 2>/dev/null
    do
        local temp=${spin#?}
        printf " [%c]  " "$spin"
        local spin=$temp${spin%"$temp"}
        sleep .1
        printf "\b\b\b\b\b\b"
    done
    printf "    \b\b\b\b"
}

cd "${0%/*}"

echo
echo "###################################################################################"
echo
echo "     This script will download source packages and repos for the libraries required for IGraphics,"
echo "     please relax and have a cup of tea, it'll take a while..."
echo
echo "###################################################################################"
echo

if [ ! -d "$BUILD_DIR" ]
then
  mkdir "$BUILD_DIR"
fi

if [ ! -d "$DL_DIR" ]
then
  mkdir "$DL_DIR"
fi

if [ ! -d "$SRC_DIR" ]
then
  mkdir "$SRC_DIR"
fi

cd "$DL_DIR"

echo

if [ -e "$LOG_PATH/$LOG_NAME" ]
then
    rm "$LOG_PATH/$LOG_NAME"
else
    touch "$LOG_PATH/$LOG_NAME"
fi

#######################################################################

#zlib
if [ -d "$SRC_DIR/zlib" ]
then
  echo "Found zlib"
else
  echo
  echo "Downloading zlib"
  if [ -e $ZLIB_VERSION.tar.gz ]
  then
    echo "Tarball Present..."
  else
    curl -L --progress-bar -O $ZLIB_URL/$ZLIB_VERSION.tar.gz
  fi
  echo "Unpacking..."
  tar -xf $ZLIB_VERSION.tar.gz
  mv $ZLIB_VERSION "$SRC_DIR/zlib"
fi

#######################################################################

#libpng
if [ -d "$SRC_DIR/libpng" ]
 then
  echo "Found libpng"
 else
  echo
  echo "Downloading libpng..."
  if [ -e $PNG_VERSION.tar.gz ]
  then
    echo "Tarball Present..."
  else
    curl -L --progress-bar -O $PNG_URL/$PNG_VERSION.tar.gz
  fi
  echo "Unpacking..."
  tar -xf $PNG_VERSION.tar.gz
  mv libpng* "$SRC_DIR/libpng"
  echo "copying pnglibconf.h"
  cp "$SRC_DIR/libpng/scripts/pnglibconf.h.prebuilt" "$SRC_DIR/libpng/pnglibconf.h"
fi

#######################################################################

#freetype
if [ -d "$SRC_DIR/freetype" ]
then
  echo "Found freetype"
else
  echo
  echo "Downloading freetype"
  if [ -e $FREETYPE_VERSION.tar.gz ]
  then
    echo "Tarball Present..."
  else
    echo "Downloading..."
    curl --progress-bar -OL --disable-epsv $FREETYPE_URL/$FREETYPE_VERSION.tar.gz
  fi
  echo "Unpacking..."
  tar -xf $FREETYPE_VERSION.tar.gz
  mv $FREETYPE_VERSION "$SRC_DIR/freetype"
fi

#######################################################################

#skia
if [ -d "$SRC_DIR/skia" ]
then
  echo "Found skia"
else
  echo "Downloading skia"
  git clone --depth 1 --branch $SKIA_VERSION $SKIA_URL "$SRC_DIR/skia"
  # git clone $SKIA_URL "$SRC_DIR/skia"
  # git checkout $SKIA_VERSION
  rm -r -f .git
  cd "$IGRAPHICS_DEPS_DIR"
fi

#rm -r $DL_DIR
