#!/usr/bin/env bash
set -e

BASE_LOCATION="$PWD/../Build"
BUILD_LOCATION="$BASE_LOCATION/src"
INSTALL_LOCATION="$BASE_LOCATION/mac"
INCLUDE_PATH="$INSTALL_LOCATION/include"
LIB_PATH="$INSTALL_LOCATION/lib"
BIN_PATH="$INSTALL_LOCATION/bin"
LOG_PATH="$BASE_LOCATION"
LOG_NAME="build-mac.log"

CAIRO_VERSION=cairo-1.16.0
FREETYPE_VERSION=freetype-2.9.1
PKGCONFIG_VERSION=pkg-config-0.28
PIXMAN_VERSION=pixman-0.34.0
EXPAT_VERSION=expat-2.2.5
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
echo "     This script will download and build libraries required for IGraphics on macOS,"
echo "     please relax and have a cup of tea, it'll take a while..."
echo
echo "###################################################################################"
echo

if [ ! -d "$BASE_LOCATION" ]
then
  mkdir "$BASE_LOCATION"
fi

if [ ! -d "$BUILD_LOCATION" ]
then
  mkdir "$BUILD_LOCATION"
fi

if [ ! -d "$INSTALL_LOCATION" ]
then
mkdir "$INSTALL_LOCATION"
fi

cd "$BUILD_LOCATION"

echo

export MACOSX_DEPLOYMENT_TARGET=10.7

export LDFLAGS="-arch i386 -arch x86_64"
export CFLAGS="-Os -arch i386 -arch x86_64"
export CXXFLAGS="-Os -arch i386 -arch x86_64"

# remove old log file if exists
if [ -e $LOG_PATH/$LOG_NAME ]
then
    rm $LOG_PATH/$LOG_NAME
else
    touch $LOG_PATH/$LOG_NAME
fi

#######################################################################

#bzip
# if [ -e $LIB_PATH/libbz2.a ]
# then
#   echo "Found bzip2"
# else
#   echo
#   echo "Installing bzip2"
#   if [ -e bzip2-1.0.6.tar.gz ]
#   then
#     echo "Tarball Present..."
#   else
#     echo "Downloading..."
#     curl -O -L --progress-bar http://www.bzip.org/1.0.6/bzip2-1.0.6.tar.gz
#   fi
#   echo "Unpacking..."
#   tar -xf bzip2-1.0.6.tar.gz >> $LOG_PATH/$LOG_NAME 2>&1
#   cd bzip2-1.0.6
#   echo -n "Building..."
#   echo "---------------------------- Build bzip2 ----------------------------" >> $LOG_PATH/$LOG_NAME 2>&1
#   make CFLAGS="$CFLAGS -Wno-format" PREFIX="$INSTALL_LOCATION" -s install >> $LOG_PATH/$LOG_NAME 2>&1 &
#   spin
#   make -s clean >> $LOG_PATH/$LOG_NAME 2>&1 &
#   spin
#   echo "done."
#   echo "bzip2 Installed!"
#   echo
#   cd "$BUILD_LOCATION"
# fi

#######################################################################

#pkg-config

if [ -e "$BIN_PATH/pkg-config" ]
then
  echo "Found pkg-config"
else
  echo "Installing pkg-config"
  if [ -e $PKGCONFIG_VERSION.tar.gz ]
  then
    echo "Tarball Present..."
  else
    echo "Downloading..."
    curl -L --progress-bar -O https://pkg-config.freedesktop.org/releases/$PKGCONFIG_VERSION.tar.gz
  fi
  echo "Unpacking..."
  tar xfz $PKGCONFIG_VERSION.tar.gz
  cd $PKGCONFIG_VERSION
  echo -n "Configuring..."
  echo "---------------------------- Configure pkg-config ----------------------------" >> $LOG_PATH/$LOG_NAME 2>&1
  ./configure CFLAGS="-Os -arch x86_64" LDFLAGS="-arch x86_64" --prefix "$INSTALL_LOCATION" --with-internal-glib >> $LOG_PATH/$LOG_NAME 2>&1 &
  spin
  echo "done."
  echo -n "Building..."
  echo "---------------------------- Build pkg-config ----------------------------" >> $LOG_PATH/$LOG_NAME 2>&1
  #make -s RUN_FC_CACHE_TEST=false -s install >> $LOG_PATH/$LOG_NAME 2>&1 &
  #spin
  make -s install >> $LOG_PATH/$LOG_NAME 2>&1 &
  #>> $LOG_PATH/$LOG_NAME 2>&1 &
  spin
  make -s clean >> $LOG_PATH/$LOG_NAME 2>&1 &
  spin
  echo "done."
  echo "pkg-config Installed!"
  echo
  cd "$BUILD_LOCATION"
fi

#######################################################################

#expat
# if [ -e "$LIB_PATH/libexpat.a" ]
# then
#   echo "Found expat"
# else
#   echo
#   echo "Installing expat"
#   if [ -e $EXPAT_VERSION.tar.bz2 ]
#   then
#     echo "Tarball Present..."
#   else
#     echo "Downloading..."
#     curl -L --progress-bar -O https://github.com/libexpat/libexpat/releases/download/R_2_2_5/$EXPAT_VERSION.tar.bz2
#   fi
#   echo "Unpacking..."
#   tar -jxf "$EXPAT_VERSION.tar.bz2"
#   cd $EXPAT_VERSION
#   echo -n "Configuring..."
#   echo "---------------------------- Configure expat ----------------------------" >> $LOG_PATH/$LOG_NAME 2>&1
#   ./configure --disable-shared --enable-static --prefix "$INSTALL_LOCATION" >> $LOG_PATH/$LOG_NAME 2>&1 &
#   spin
#   echo "done."
#   echo -n "Building..."
#   echo "---------------------------- Build expat ----------------------------" >> $LOG_PATH/$LOG_NAME 2>&1
#   make -s install >> $LOG_PATH/$LOG_NAME 2>&1 &
#   spin
#   make -s clean >> $LOG_PATH/$LOG_NAME 2>&1 &
#   spin
#   echo "done."
#   echo
#   echo "expat Installed!"
#   echo
#   cd "$BUILD_LOCATION"
# fi
#######################################################################

#zlib
if [ -e "$LIB_PATH/libz.a" ]
then
  echo "Found zlib"
else
  COPTZL="-Wno-shift-negative-value"
  echo
  echo "Installing zlib"
  if [ -e $ZLIB_VERSION.tar.gz ]
  then
    echo "Tarball Present..."
  else
    echo "Downloading..."
    curl -L --progress-bar -O https://www.zlib.net/$ZLIB_VERSION.tar.gz
  fi
  echo "Unpacking..."
  tar -xf $ZLIB_VERSION.tar.gz
  cd $ZLIB_VERSION
  echo -n "Configuring..."
  echo "---------------------------- Configure zlib ----------------------------" >> $LOG_PATH/$LOG_NAME 2>&1
  ./configure --static --archs="-arch i386 -arch x86_64" --prefix "$INSTALL_LOCATION" >> $LOG_PATH/$LOG_NAME 2>&1 &
  spin
  echo "done."
  echo -n "Building..."
  echo "---------------------------- Build zlib ----------------------------" >> $LOG_PATH/$LOG_NAME 2>&1
  make CFLAGS="$CFLAGS $COPTZL" -s install >> $LOG_PATH/$LOG_NAME 2>&1 &
  spin
  make -s clean >> $LOG_PATH/$LOG_NAME 2>&1 &
  spin
  echo "done."
  echo
  echo "zlib Installed!"
  echo
  cd "$BUILD_LOCATION"
fi

#######################################################################

#libpng
if [ -e "$LIB_PATH/libpng16.a" ]
then
  echo "Found libpng"
else
  echo
  echo "Installing libpng"
  if [ -e $PNG_VERSION.tar.xz ]
  then
    echo "Tarball Present..."
  else
    echo "Downloading..."
    curl -L --progress-bar -O http://github.com/glennrp/libpng-releases/raw/master/$PNG_VERSION.tar.xz
  fi
  echo "Unpacking..."
  tar -xf $PNG_VERSION.tar.xz
  cd $PNG_VERSION
  echo -n "Configuring..."
  echo "---------------------------- Configure libpng ----------------------------" >> $LOG_PATH/$LOG_NAME 2>&1
  ./configure --disable-dependency-tracking --enable-static --disable-shared --prefix "$INSTALL_LOCATION" >> $LOG_PATH/$LOG_NAME 2>&1 &
  spin
  echo "done."
  echo -n "Building..."
  echo "---------------------------- Build libpng ----------------------------" >> $LOG_PATH/$LOG_NAME 2>&1
  make -s install  >> $LOG_PATH/$LOG_NAME 2>&1 &
  spin
  make -s clean >> $LOG_PATH/$LOG_NAME 2>&1 &
  spin
  echo "done."
  echo
  echo "libpng Installed!"
  echo
  cd "$BUILD_LOCATION"
fi

#######################################################################

#pixman
if [ -e "$LIB_PATH/libpixman-1.a" ]
then
  echo "Found pixman"
else
  COPTPX="-Wno-unknown-attributes -Wno-unused-command-line-argument -Wno-shift-negative-value -Wno-tautological-constant-out-of-range-compare"
  echo
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
  cd $PIXMAN_VERSION
  echo -n "Configuring..."
  echo "---------------------------- Configure pixman ----------------------------" >> $LOG_PATH/$LOG_NAME 2>&1
  ./configure --enable-static --disable-dependency-tracking --disable-gtk --prefix "$INSTALL_LOCATION" PKG_CONFIG="$BIN_PATH/pkg-config" PKG_CONFIG_LIBDIR="$LIB_PATH/pkgconfig" >> $LOG_PATH/$LOG_NAME 2>&1 &
  spin
  echo "done."
  echo -n "Building..."
  echo "---------------------------- Build pixman ----------------------------" >> $LOG_PATH/$LOG_NAME 2>&1
  make CFLAGS="-DHAVE_CONFIG_H $COPTPX $CFLAGS" -s install >> $LOG_PATH/$LOG_NAME 2>&1 &
  spin
  make -s clean >> $LOG_PATH/$LOG_NAME 2>&1 &
  spin
  echo "done."
  echo
  cp pixman-1.pc "$LIB_PATH/pkgconfig/pixman-1.pc"
  #Must remove this after build, as building without them fails
  rm "$LIB_PATH/libpixman-1.0.dylib"
  rm "$LIB_PATH/libpixman-1.dylib"
  echo "pixman Installed!"
  echo
  cd "$BUILD_LOCATION"
fi

#######################################################################

#freetype
if [ -e "$LIB_PATH/libfreetype.a" ]
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
    curl  --progress-bar -OL --disable-epsv https://download.savannah.gnu.org/releases/freetype/$FREETYPE_VERSION.tar.gz
  fi
  echo "Unpacking..."
  tar -xf $FREETYPE_VERSION.tar.gz
  cd $FREETYPE_VERSION
  echo -n "Configuring..."
  echo "---------------------------- Configure freetype ----------------------------" >> $LOG_PATH/$LOG_NAME 2>&1
  ./configure --prefix "$INSTALL_LOCATION" --disable-shared --enable-biarch-config --without-zlib --without-bzip2 PKG_CONFIG="$BIN_PATH/pkg-config" PKG_CONFIG_LIBDIR="$LIB_PATH/pkgconfig" >> $LOG_PATH/$LOG_NAME 2>&1 &
  spin
  echo "done."
  echo -n "Building..."
  echo "---------------------------- Build freetype ----------------------------" >> $LOG_PATH/$LOG_NAME 2>&1
  make -s install >> $LOG_PATH/$LOG_NAME 2>&1 &
  spin
  make -s clean >> $LOG_PATH/$LOG_NAME 2>&1 &
  spin
  echo "done."
  echo
  echo "freetype Installed!"
  echo
  cd "$BUILD_LOCATION"
fi

#######################################################################

#fontconfig

# if [ -e "$LIB_PATH/libfontconfig.a" ]
# then
#   echo "Found fontconfig"
# else
#   COPTFC="-Wno-macro-redefined -Wno-unused-command-line-argument -Wno-non-literal-null-conversion -Wno-pointer-bool-conversion -Wno-unused-function"
#   echo
#   echo "Installing fontconfig"
#   if [ -e fontconfig-2.12.6.tar.bz2 ] || [ -e fontconfig-2.12.6.tar ]
#   then
#     echo "Tarball Present..."
#   else
#     echo "Downloading..."
#     curl -L --progress-bar -O https://www.freedesktop.org/software/fontconfig/release/fontconfig-2.12.6.tar.bz2
#   fi
#   echo "Unpacking..."
#   if [ ! -e fontconfig-2.12.6.tar ]
#   then
#     "$INSTALL_LOCATION/bin/bunzip2" "$BUILD_LOCATION/fontconfig-2.12.6.tar.bz2"
#   fi
#   tar -xf fontconfig-2.12.6.tar
#   cd fontconfig-2.12.6
#   echo -n "Configuring..."
#   echo "---------------------------- Configure fontconfig ----------------------------" >> $LOG_PATH/$LOG_NAME 2>&1
#   ./configure --disable-dependency-tracking --disable-shared --enable-static --silent CFLAGS="$CFLAGS $COPTFC" --prefix "$INSTALL_LOCATION" LDFLAGS="$LDFLAGS -L$LIB_PATH" LIBS="-lbz2" PKG_CONFIG="$BIN_PATH/pkg-config" PKG_CONFIG_LIBDIR="$LIB_PATH/pkgconfig" >> $LOG_PATH/$LOG_NAME 2>&1 &
#   spin
#   echo "done."
#   echo -n "Building..."
#   echo "---------------------------- Build fontconfig ----------------------------" >> $LOG_PATH/$LOG_NAME 2>&1
#   make -s RUN_FC_CACHE_TEST=false -s install >> $LOG_PATH/$LOG_NAME 2>&1 &
#   spin
#   make -s clean >> $LOG_PATH/$LOG_NAME 2>&1 2>&1 &
#   spin
#   echo "done."
#   echo "fontconfig Installed!"
#   echo
#   cd "$BUILD_LOCATION"
# fi

#######################################################################

#cairo
if [ -e "$LIB_PATH/libcairo.a" ]
then
  echo "Found cairo"
else
  COPTCR="-Wno-logical-not-parentheses -Wno-parentheses-equality -Wno-enum-conversion -Wno-unused-command-line-argument -Wno-unused-function -Wno-unused-variable -Wno-unused-local-typedef -Wno-tautological-constant-out-of-range-compare -Wno-absolute-value -Wno-literal-conversion"
  echo
  echo "Installing cairo"
  if [ -e $CAIRO_VERSION.tar.xz ]
  then
    echo "Tarball Present..."
  else
    echo "Downloading..."
    curl -L --progress-bar -O https://cairographics.org/releases/$CAIRO_VERSION.tar.xz
  fi
  echo "Unpacking..."
  tar -xf $CAIRO_VERSION.tar.xz
  cd $CAIRO_VERSION
  echo -n "Configuring..."
  echo "---------------------------- Configure cairo ----------------------------" >> $LOG_PATH/$LOG_NAME 2>&1
  ./configure --disable-shared --enable-static --disable-dependency-tracking --disable-svg --disable-pdf --disable-ps --disable-fc --enable-quartz-image=yes --disable-interpreter --disable-trace CFLAGS="$CFLAGS $COPTCR" --prefix "$INSTALL_LOCATION" PKG_CONFIG="$BIN_PATH/pkg-config" PKG_CONFIG_LIBDIR="$LIB_PATH/pkgconfig" LDFLAGS="$LDFLAGS -framework CoreFoundation -framework CoreGraphics -framework CoreText" >> $LOG_PATH/$LOG_NAME 2>&1 &
  spin
  echo "done."
  echo -n "Building..."
  echo "---------------------------- Build cairo ----------------------------" >> $LOG_PATH/$LOG_NAME 2>&1
  make -s install >> $LOG_PATH/$LOG_NAME 2>&1 &
  spin
  make -s clean >> $LOG_PATH/$LOG_NAME 2>&1 &
  spin
  echo "done."
  echo "cairo Installed!"
  echo
  cd "$BUILD_LOCATION"
fi

#remove unwanted dirs
#rm -r $INSTALL_LOCATION/man/
#rm -r $INSTALL_LOCATION/share/
#rm -r $INSTALL_LOCATION/bin/

echo "Verify UB Builds..."
# file "$LIB_PATH/libbz2.a"
# file "$LIB_PATH/libexpat.a"
file "$LIB_PATH/libz.a"
file "$LIB_PATH/libpixman-1.a"
file "$LIB_PATH/libpng16.a"
file "$LIB_PATH/libfreetype.a"
# file "$LIB_PATH/libfontconfig.a"
file "$LIB_PATH/libcairo.a"
exit

#rm -r $BUILD_LOCATION
