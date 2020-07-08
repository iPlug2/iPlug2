#!/usr/bin/env bash
set -e

IGRAPHICS_DEPS_DIR="$PWD"
BUILD_DIR="$IGRAPHICS_DEPS_DIR/../Build"
SRC_DIR="$BUILD_DIR/src"
DL_DIR="$BUILD_DIR/tmp"
INSTALL_DIR="$BUILD_DIR/mac"
INCLUDE_DIR="$INSTALL_DIR/include"
LIB_DIR="$INSTALL_DIR/lib"
BIN_DIR="$INSTALL_DIR/bin"
LOG_DIR="$BUILD_DIR"
LOG_NAME="build-mac.log"

CAIRO_VERSION=cairo-1.16.0
FREETYPE_VERSION=freetype-2.9.1
PKGCONFIG_VERSION=pkg-config-0.28
PIXMAN_VERSION=pixman-0.34.0
EXPAT_VERSION=expat-2.2.5
PNG_VERSION=libpng-1.6.34
ZLIB_VERSION=zlib-1.2.11

[[ -e "$PWD/build-igraphics-libs-mac.sh" ]] ||
{
  echo "*******************************************************************************"
  echo "Error: Please cd into the folder containing this script before running it.";
  echo "*******************************************************************************"
  exit 1;
}

err_report() {
    echo
    echo "*******************************************************************************"
    echo "Error: something went wrong during the build process, printing $LOG_NAME "
    echo "*******************************************************************************"
    echo
    cat $LOG_DIR/$LOG_NAME
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

if [ ! -d "$INSTALL_DIR" ]
then
  mkdir "$INSTALL_DIR"
fi

cd "$SRC_DIR"

echo

# export MACOSX_DEPLOYMENT_TARGET=10.7
export MACOSX_DEPLOYMENT_TARGET=10.9

# export LDFLAGS="-arch i386 -arch x86_64"
# export CFLAGS="-Os -arch i386 -arch x86_64"
# export CXXFLAGS="-Os -arch i386 -arch x86_64"
export LDFLAGS="-arch x86_64"
export CFLAGS="-Os -arch x86_64"
export CXXFLAGS="-Os -arch x86_64"

# remove old log file if exists
if [ -e $LOG_DIR/$LOG_NAME ]
then
    rm $LOG_DIR/$LOG_NAME
else
    touch $LOG_DIR/$LOG_NAME
fi

#######################################################################

#bzip
# if [ -e $LIB_DIR/libbz2.a ]
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
#   tar -xf bzip2-1.0.6.tar.gz >> $LOG_DIR/$LOG_NAME 2>&1
#   cd bzip2-1.0.6
#   echo -n "Building..."
#   echo "---------------------------- Build bzip2 ----------------------------" >> $LOG_DIR/$LOG_NAME 2>&1
#   make CFLAGS="$CFLAGS -Wno-format" PREFIX="$INSTALL_DIR" -s install >> $LOG_DIR/$LOG_NAME 2>&1 &
#   spin
#   make -s clean >> $LOG_DIR/$LOG_NAME 2>&1 &
#   spin
#   echo "done."
#   echo "bzip2 Installed!"
#   echo
#   cd "$SRC_DIR"
# fi

#######################################################################

#pkg-config

if [ -e "$BIN_DIR/pkg-config" ]
then
  echo "Found pkg-config"
else
  echo "Installing pkg-config"
  cd $DL_DIR
  if [ -e "$PKGCONFIG_VERSION.tar.gz" ]
  then
    echo "Tarball Present..."
  else
    echo "Downloading..."
    curl -L --progress-bar -O https://pkg-config.freedesktop.org/releases/$PKGCONFIG_VERSION.tar.gz
  fi
  echo "Unpacking..."
  tar xfz $PKGCONFIG_VERSION.tar.gz
  mv $PKGCONFIG_VERSION "$SRC_DIR/pkgconfig"

  echo -n "Configuring..."
  cd "$SRC_DIR/pkgconfig"
  echo "---------------------------- Configure pkg-config ----------------------------" >> $LOG_DIR/$LOG_NAME 2>&1
  ./configure CFLAGS="-Os -arch x86_64" LDFLAGS="-arch x86_64" --prefix "$INSTALL_DIR" --with-internal-glib >> $LOG_DIR/$LOG_NAME 2>&1 &
  spin
  echo "done."
  echo -n "Building..."
  echo "---------------------------- Build pkg-config ----------------------------" >> $LOG_DIR/$LOG_NAME 2>&1
  #make -s RUN_FC_CACHE_TEST=false -s install >> $LOG_DIR/$LOG_NAME 2>&1 &
  #spin
  make -s install >> $LOG_DIR/$LOG_NAME 2>&1 &
  #>> $LOG_DIR/$LOG_NAME 2>&1 &
  spin
  make -s clean >> $LOG_DIR/$LOG_NAME 2>&1 &
  spin
  echo "done."
  echo "pkg-config Installed!"
  echo
  cd "$SRC_DIR"
fi

#######################################################################

#expat
# if [ -e "$LIB_DIR/libexpat.a" ]
# then
#   echo "Found expat"
# else
#   echo
#   echo "Installing expat"
#   cd $DL_DIR
#   if [ -e $EXPAT_VERSION.tar.bz2 ]
#   then
#     echo "Tarball Present..."
#   else
#     echo "Downloading..."
#     curl -L --progress-bar -O https://github.com/libexpat/libexpat/releases/download/R_2_2_5/$EXPAT_VERSION.tar.bz2
#   fi
#   echo "Unpacking..."
#   tar -jxf "$EXPAT_VERSION.tar.bz2"
#   mv $EXPAT_VERSION "$SRC_DIR/expat"

#   echo -n "Configuring..."
#   cd "$SRC_DIR/expat"
#   echo "---------------------------- Configure expat ----------------------------" >> $LOG_DIR/$LOG_NAME 2>&1
#   ./configure --disable-shared --enable-static --prefix "$INSTALL_DIR" >> $LOG_DIR/$LOG_NAME 2>&1 &
#   spin
#   echo "done."
#   echo -n "Building..."
#   echo "---------------------------- Build expat ----------------------------" >> $LOG_DIR/$LOG_NAME 2>&1
#   make -s install >> $LOG_DIR/$LOG_NAME 2>&1 &
#   spin
#   make -s clean >> $LOG_DIR/$LOG_NAME 2>&1 &
#   spin
#   echo "done."
#   echo
#   echo "expat Installed!"
#   echo
#   cd "$SRC_DIR"
# fi
#######################################################################

#zlib
if [ -e "$LIB_DIR/libz.a" ]
then
  echo "Found zlib"
else
  COPTZL="-Wno-shift-negative-value"
  echo
  echo "Installing zlib"
  cd $DL_DIR
  if [ -e $ZLIB_VERSION.tar.gz ]
  then
    echo "Tarball Present..."
  else
    echo "Downloading..."
    curl -L --progress-bar -O https://www.zlib.net/$ZLIB_VERSION.tar.gz
  fi
  echo "Unpacking..."
  tar -xf $ZLIB_VERSION.tar.gz
  mv $ZLIB_VERSION "$SRC_DIR/zlib"

  echo -n "Configuring..."
  cd "$SRC_DIR/zlib"
  echo "---------------------------- Configure zlib ----------------------------" >> $LOG_DIR/$LOG_NAME 2>&1
  ./configure --static --archs="-arch i386 -arch x86_64" --prefix "$INSTALL_DIR" >> $LOG_DIR/$LOG_NAME 2>&1 &
  spin
  echo "done."
  echo -n "Building..."
  echo "---------------------------- Build zlib ----------------------------" >> $LOG_DIR/$LOG_NAME 2>&1
  make CFLAGS="$CFLAGS $COPTZL" -s install >> $LOG_DIR/$LOG_NAME 2>&1 &
  spin
  make -s clean >> $LOG_DIR/$LOG_NAME 2>&1 &
  spin
  echo "done."
  echo
  echo "zlib Installed!"
  echo
  cd "$SRC_DIR"
fi

#######################################################################

#libpng
if [ -e "$LIB_DIR/libpng16.a" ]
then
  echo "Found libpng"
else
  echo
  echo "Installing libpng"
  cd $DL_DIR
  if [ -e $PNG_VERSION.tar.xz ]
  then
    echo "Tarball Present..."
  else
    echo "Downloading..."
    curl -L --progress-bar -O http://github.com/glennrp/libpng-releases/raw/master/$PNG_VERSION.tar.xz
  fi
  echo "Unpacking..."
  tar -xf $PNG_VERSION.tar.xz
  mv $PNG_VERSION "$SRC_DIR/libpng"

  echo -n "Configuring..."
  cd "$SRC_DIR/libpng"
  echo "---------------------------- Configure libpng ----------------------------" >> $LOG_DIR/$LOG_NAME 2>&1
  ./configure --disable-dependency-tracking --enable-static --disable-shared --prefix "$INSTALL_DIR" >> $LOG_DIR/$LOG_NAME 2>&1 &
  spin
  echo "done."
  echo -n "Building..."
  echo "---------------------------- Build libpng ----------------------------" >> $LOG_DIR/$LOG_NAME 2>&1
  make -s install  >> $LOG_DIR/$LOG_NAME 2>&1 &
  spin
  make -s clean >> $LOG_DIR/$LOG_NAME 2>&1 &
  spin
  echo "done."
  echo
  echo "libpng Installed!"
  echo
  cd "$SRC_DIR"
fi

#######################################################################

#pixman
if [ -e "$LIB_DIR/libpixman-1.a" ]
then
  echo "Found pixman"
else
  COPTPX="-Wno-unknown-attributes -Wno-unused-command-line-argument -Wno-shift-negative-value -Wno-tautological-constant-out-of-range-compare"
  echo
  echo "Installing pixman"
  cd $DL_DIR
  if [ -e $PIXMAN_VERSION.tar.gz ]
  then
    echo "Tarball Present..."
  else
    echo "Downloading..."
    curl -L --progress-bar -O https://cairographics.org/releases/$PIXMAN_VERSION.tar.gz
  fi
  echo "Unpacking..."
  tar -xf $PIXMAN_VERSION.tar.gz
  mv $PIXMAN_VERSION "$SRC_DIR/pixman"

  echo -n "Configuring..."
  cd "$SRC_DIR/pixman"
  echo "---------------------------- Configure pixman ----------------------------" >> $LOG_DIR/$LOG_NAME 2>&1
  ./configure --enable-static --disable-dependency-tracking --disable-gtk --prefix "$INSTALL_DIR" PKG_CONFIG="$BIN_DIR/pkg-config" PKG_CONFIG_LIBDIR="$LIB_DIR/pkgconfig" >> $LOG_DIR/$LOG_NAME 2>&1 &
  spin
  echo "done."
  echo -n "Building..."
  echo "---------------------------- Build pixman ----------------------------" >> $LOG_DIR/$LOG_NAME 2>&1
  make CFLAGS="-DHAVE_CONFIG_H $COPTPX $CFLAGS" -s install >> $LOG_DIR/$LOG_NAME 2>&1 &
  spin
  make -s clean >> $LOG_DIR/$LOG_NAME 2>&1 &
  spin
  echo "done."
  echo
  cp pixman-1.pc "$LIB_DIR/pkgconfig/pixman-1.pc"
  #Must remove this after build, as building without them fails
  rm "$LIB_DIR/libpixman-1.0.dylib"
  rm "$LIB_DIR/libpixman-1.dylib"
  echo "pixman Installed!"
  echo
  cd "$SRC_DIR"
fi

#######################################################################

#freetype
if [ -e "$LIB_DIR/libfreetype.a" ]
then
  echo "Found freetype"
else
  echo
  echo "Installing freetype"
  cd $DL_DIR
  if [ -e $FREETYPE_VERSION.tar.gz ]
  then
    echo "Tarball Present..."
  else
    echo "Downloading..."
    curl  --progress-bar -OL --disable-epsv https://download.savannah.gnu.org/releases/freetype/$FREETYPE_VERSION.tar.gz
  fi
  echo "Unpacking..."
  tar -xf $FREETYPE_VERSION.tar.gz
  mv $FREETYPE_VERSION "$SRC_DIR/freetype"

  echo -n "Configuring..."
  cd "$SRC_DIR/freetype"
  echo "---------------------------- Configure freetype ----------------------------" >> $LOG_DIR/$LOG_NAME 2>&1
  ./configure --prefix "$INSTALL_DIR" --disable-shared --enable-biarch-config --without-zlib --without-bzip2 PKG_CONFIG="$BIN_DIR/pkg-config" PKG_CONFIG_LIBDIR="$LIB_DIR/pkgconfig" >> $LOG_DIR/$LOG_NAME 2>&1 &
  spin
  echo "done."
  echo -n "Building..."
  echo "---------------------------- Build freetype ----------------------------" >> $LOG_DIR/$LOG_NAME 2>&1
  make -s install >> $LOG_DIR/$LOG_NAME 2>&1 &
  spin
  make -s clean >> $LOG_DIR/$LOG_NAME 2>&1 &
  spin
  echo "done."
  echo
  echo "freetype Installed!"
  echo
  cd "$SRC_DIR"
fi

#######################################################################

#fontconfig

# if [ -e "$LIB_DIR/libfontconfig.a" ]
# then
#   echo "Found fontconfig"
# else
#   COPTFC="-Wno-macro-redefined -Wno-unused-command-line-argument -Wno-non-literal-null-conversion -Wno-pointer-bool-conversion -Wno-unused-function"
#   echo
#   echo "Installing fontconfig"
#   cd $DL_DIR
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
#     "$INSTALL_DIR/bin/bunzip2" "$SRC_DIR/fontconfig-2.12.6.tar.bz2"
#   fi
#   tar -xf fontconfig-2.12.6.tar
#   mv fontconfig-2.12.6. "$SRC_DIR/fontconfig"

#   echo -n "Configuring..."
#   cd "$SRC_DIR/fontconfig"
#   echo "---------------------------- Configure fontconfig ----------------------------" >> $LOG_DIR/$LOG_NAME 2>&1
#   ./configure --disable-dependency-tracking --disable-shared --enable-static --silent CFLAGS="$CFLAGS $COPTFC" --prefix "$INSTALL_DIR" LDFLAGS="$LDFLAGS -L$LIB_DIR" LIBS="-lbz2" PKG_CONFIG="$BIN_DIR/pkg-config" PKG_CONFIG_LIBDIR="$LIB_DIR/pkgconfig" >> $LOG_DIR/$LOG_NAME 2>&1 &
#   spin
#   echo "done."
#   echo -n "Building..."
#   echo "---------------------------- Build fontconfig ----------------------------" >> $LOG_DIR/$LOG_NAME 2>&1
#   make -s RUN_FC_CACHE_TEST=false -s install >> $LOG_DIR/$LOG_NAME 2>&1 &
#   spin
#   make -s clean >> $LOG_DIR/$LOG_NAME 2>&1 2>&1 &
#   spin
#   echo "done."
#   echo "fontconfig Installed!"
#   echo
#   cd "$SRC_DIR"
# fi

#######################################################################

#cairo
if [ -e "$LIB_DIR/libcairo.a" ]
then
  echo "Found cairo"
else
  COPTCR="-Wno-logical-not-parentheses -Wno-parentheses-equality -Wno-enum-conversion -Wno-unused-command-line-argument -Wno-unused-function -Wno-unused-variable -Wno-unused-local-typedef -Wno-tautological-constant-out-of-range-compare -Wno-absolute-value -Wno-literal-conversion"
  echo
  echo "Installing cairo"
  cd $DL_DIR
  if [ -e $CAIRO_VERSION.tar.xz ]
  then
    echo "Tarball Present..."
  else
    echo "Downloading..."
    curl -L --progress-bar -O https://cairographics.org/releases/$CAIRO_VERSION.tar.xz
  fi
  echo "Unpacking..."
  tar -xf $CAIRO_VERSION.tar.xz
  mv $CAIRO_VERSION "$SRC_DIR/cairo"
  cd "$SRC_DIR/cairo"
  echo "Patching for macOS 10.13 compiliation..."
  sed -i.bu 's/if test "x$cairo_cc_stderr" != "x"; then/if 0; then/' configure

  echo -n "Configuring..."
  echo "---------------------------- Configure cairo ----------------------------" >> $LOG_DIR/$LOG_NAME 2>&1
  ./configure --disable-shared --enable-static --disable-dependency-tracking --disable-svg --disable-pdf --disable-ps --disable-fc --enable-quartz-image=yes --disable-interpreter --disable-trace CFLAGS="$CFLAGS $COPTCR" --prefix "$INSTALL_DIR" PKG_CONFIG="$BIN_DIR/pkg-config" PKG_CONFIG_LIBDIR="$LIB_DIR/pkgconfig" LDFLAGS="$LDFLAGS -framework CoreFoundation -framework CoreGraphics -framework CoreText" >> $LOG_DIR/$LOG_NAME 2>&1 &
  spin
  echo "done."
  echo -n "Building..."
  echo "---------------------------- Build cairo ----------------------------" >> $LOG_DIR/$LOG_NAME 2>&1
  make -s install >> $LOG_DIR/$LOG_NAME 2>&1 &
  spin
  make -s clean >> $LOG_DIR/$LOG_NAME 2>&1 &
  spin
  echo "done."
  echo "cairo Installed!"
  echo
  cd "$SRC_DIR"
fi

#remove unwanted dirs
#rm -r $INSTALL_DIR/man/
#rm -r $INSTALL_DIR/share/
#rm -r $INSTALL_DIR/bin/

# echo "Verify UB Builds..."
# file "$LIB_DIR/libbz2.a"
# file "$LIB_DIR/libexpat.a"
# file "$LIB_DIR/libz.a"
# file "$LIB_DIR/libpixman-1.a"
# file "$LIB_DIR/libpng16.a"
# file "$LIB_DIR/libfreetype.a"
# file "$LIB_DIR/libfontconfig.a"
# file "$LIB_DIR/libcairo.a"
exit

#rm -r $SRC_DIR
