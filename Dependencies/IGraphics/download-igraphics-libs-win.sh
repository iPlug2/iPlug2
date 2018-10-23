#!/usr/bin/env bash
set -e

IGRAPHICS_DIR="$PWD"
BUILD_DIR="$PWD/../Build"
DL_DIR="$BUILD_DIR/src"
INSTALL_DIR="$BUILD_DIR/win"
LOG_PATH="$BUILD_DIR"
LOG_NAME="build-win.log"

# CAIRO_VERSION=cairo-1.15.12
FREETYPE_VERSION=freetype-2.9
PIXMAN_VERSION=pixman-0.34.0
PNG_VERSION=libpng-1.6.34
ZLIB_VERSION=zlib-1.2.11

err_report() {
    echo
    echo "*******************************************************************************"
    echo "Error: something went wrong during the build process, printing $LOG_NAME "
    echo "*******************************************************************************"
    echo
    cat $LOG_PATH/$LOG_NAME
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

##echo "CFLAGS $CFLAGS"

echo
echo "###################################################################################"
echo
echo "     This script will download and build libraries required for IGraphics on windows,"
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

if [ ! -d "$INSTALL_DIR" ]
then
mkdir "$INSTALL_DIR"
fi

cd "$DL_DIR"

echo

# remove old log file if exists
if [ -e $LOG_PATH/$LOG_NAME ]
then
    rm $LOG_PATH/$LOG_NAME
else
    touch $LOG_PATH/$LOG_NAME
fi

#######################################################################

#zlib
# if [ -d "zlib" ]
# then
  # echo "Found zlib"
# else
  # echo "Installing zlib"
  # if [ -e $ZLIB_VERSION.tar.gz ]
  # then
    # echo "Tarball Present..."
  # else
    # echo "Downloading zlib"
    # curl -L --progress-bar -O https://www.zlib.net/$ZLIB_VERSION.tar.gz
  # fi
  # echo "Unpacking zlib..."
  # tar -xf $ZLIB_VERSION.tar.gz
  # mv $ZLIB_VERSION $IGRAPHICS_DIR/libpng/zlib
# fi

#######################################################################

#libpng
# if [ -d "libpng" ]
 # then
   # echo "Found libpng"
 # else
# echo
# echo "Installing libpng"
  # git clone https://git.code.sf.net/p/libpng/code libpng
  # cd libpng
  # git checkout -b build libpng-1.6.9-signed
  # cd ..
  # mv libpng $IGRAPHICS_DIR/libpng/libpng
# fi
  
#######################################################################

#pixman
if [ -d "pixman" ]
 then
   echo "Found pixman"
 else
  echo "Installing pixman"
  if [ -e $PIXMAN_VERSION.tar.gz ]
  then
    echo "Tarball Present..."
  else
    echo "Downloading..."
    curl -L --progress-bar -O https://cairographics.org/releases/$PIXMAN_VERSION.tar.gz
  fi
  echo "Unpacking..."
  tar -xf $PIXMAN_VERSION.tar.gz
  
  mv $PIXMAN_VERSION $IGRAPHICS_DIR/cairo/pixman
fi

#######################################################################

#freetype
if [ -d "freetype" ]
then
  echo "Found freetype"
else
  echo
  echo "Installing freetype"
  if [ -e $FREETYPE_VERSION.tar.gz ]
  then
    echo "Tarball Present..."
  else
    echo "Downloading..."
    curl --progress-bar -OL --disable-epsv https://download.savannah.gnu.org/releases/freetype/$FREETYPE_VERSION.tar.gz
  fi
  echo "Unpacking..."
  tar -xf $FREETYPE_VERSION.tar.gz
  mv $FREETYPE_VERSION $IGRAPHICS_DIR/freetype/freetype
fi

#######################################################################

#cairo
if [ -d "cairo" ]
then
  echo "Found cairo"
else
  echo "Installing cairo"
  git clone git://git.cairographics.org/git/cairo cairo
  cd cairo
  git checkout -b build 1.16.0
  cd ..
  mv cairo $IGRAPHICS_DIR/cairo/cairo
fi


#rm -r $DL_DIR
