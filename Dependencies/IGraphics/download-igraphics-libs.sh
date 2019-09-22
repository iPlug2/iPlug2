#!/usr/bin/env bash
set -e

IGRAPHICS_DEPS_DIR="$PWD"
BUILD_DIR="$IGRAPHICS_DEPS_DIR/../Build"
DL_DIR="$BUILD_DIR/tmp"
SRC_DIR="$BUILD_DIR/src"
LOG_PATH="$BUILD_DIR"
LOG_NAME="download.log"

# Basename part of tarballs to download
CAIRO_VERSION=cairo-1.16.0
FREETYPE_VERSION=freetype-2.9.1
PKGCONFIG_VERSION=pkg-config-0.28
PIXMAN_VERSION=pixman-0.34.0
EXPAT_VERSION=expat-2.2.5
PNG_VERSION=v1.6.35
ZLIB_VERSION=zlib-1.2.11
SKIA_VERSION=chrome/m78
RESVG_VERSION=v0.8.0
HB_VERSION=harfbuzz-2.6.1

# URLs where tarballs of releases can be downloaded - no trailing slash
#CAIRO tarball is compressed using xz which is not available on git-bash shell, so checkout tag via git
CAIRO_URL=https://cairographics.org/releases/
PNG_URL=https://github.com/glennrp/libpng/archive
ZLIB_URL=https://www.zlib.net
PIXMAN_URL=https://cairographics.org/releases
FREETYPE_URL=https://download.savannah.gnu.org/releases/freetype
SKIA_URL=https://github.com/google/skia.git
RESVG_URL=https://github.com/RazrFalcon/resvg.git
HB_URL=https://www.freedesktop.org/software/harfbuzz/release

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
echo "     This script will download and extract libraries required for IGraphics,"
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

#pixman
if [ -d "$SRC_DIR/pixman" ]
 then
   echo "Found pixman"
 else
  echo "Downloading pixman"
  if [ -e $PIXMAN_VERSION.tar.gz ]
  then
    echo "Tarball Present..."
  else
    echo "Downloading..."
    curl -L --progress-bar -O $PIXMAN_URL/$PIXMAN_VERSION.tar.gz
  fi
  echo "Unpacking..."
  tar -xf $PIXMAN_VERSION.tar.gz
  mv $PIXMAN_VERSION "$SRC_DIR/pixman"
fi

#######################################################################

#freetype
if [ -d "$SRC_DIR/freetype" ]
then
  echo "Found freetype"
else
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

#cairo
if [ -d "$SRC_DIR/cairo" ]
then
  echo "Found cairo"
else
  echo "Downloading cairo"
  if [ -e $CAIRO_VERSION.tar.gz ]
  then
    echo "Tarball Present..."
  else
    echo "Downloading..."
    curl --progress-bar -OL $CAIRO_URL/$CAIRO_VERSION.tar.xz
  fi
  echo "Unpacking..."
  tar -xf $CAIRO_VERSION.tar.xz
  mv $CAIRO_VERSION "$SRC_DIR/cairo"
fi

#######################################################################

#skia
if [ -d "$SRC_DIR/skia" ]
then
  echo "Found skia"
else
  echo "Downloading skia"
  git clone $SKIA_URL "$SRC_DIR/skia"
  cd "$SRC_DIR/skia"
  git checkout $SKIA_VERSION
  rm -r -f .git
  cd "$IGRAPHICS_DEPS_DIR"
fi

#######################################################################

#harfbuzz
if [ -d "$SRC_DIR/harfbuzz" ]
then
  echo "Found harfbuzz"
else
  echo
  echo "Downloading harfbuzz"
  if [ -e $HB_VERSION.tar.xz ]
  then
    echo "Tarball Present..."
  else
    echo "Downloading..."
    curl --progress-bar -OL --disable-epsv $HB_URL/$HB_VERSION.tar.xz
  fi
  echo "Unpacking..."
  tar -xf $HB_VERSION.tar.xz
  mv $HB_VERSION "$SRC_DIR/harfbuzz"
  cd "$IGRAPHICS_DEPS_DIR"
fi

#######################################################################

#resvg
if [ -d "$SRC_DIR/resvg" ]
then
  echo "Found resvg"
else
  echo "Downloading resvg"
  git clone $RESVG_URL "$SRC_DIR/resvg"
  cd "$SRC_DIR/resvg"
  git checkout $RESVG_VERSION
  rm -r -f .git
  cd "$IGRAPHICS_DEPS_DIR"
fi

#rm -r $DL_DIR
