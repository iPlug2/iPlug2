#!/usr/bin/env bash
set -e

err_report() {
    echo
    echo "*******************************************************************************"
    echo "Error: something went wrong during the build process, have a look to build.log "
    echo "*******************************************************************************"
    echo
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


BUILD_LOCATION="$PWD/build"
INSTALL_LOCATION="$PWD/install"
INCLUDE_PATH="$INSTALL_LOCATION/include"
LIB_PATH="$INSTALL_LOCATION/lib"
BIN_PATH="$INSTALL_LOCATION/bin"
LOG_PATH="$PWD"

##echo "CFLAGS $CFLAGS"

echo
echo "###################################################################################"
echo
echo "     This script will download and build libraries required for IPlug/IGraphics on macOS,"
echo "     please relax and have a cup of tea, it'll take a while..."
echo
echo "###################################################################################"
echo

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
if [ -e $LOG_PATH/build.log ]
then
    rm $LOG_PATH/build.log
else
    touch $LOG_PATH/build.log
fi

#######################################################################

#bzip
if [ -e $LIB_PATH/libbz2.a ]
then
  echo "Found bzip2"
else
  echo
  echo "Installing bzip2"
  if [ -e bzip2-1.0.6.tar.gz ]
  then
    echo "Tarball Present..."
  else
    echo "Downloading..."
    curl -L --progress-bar -O http://www.bzip.org/1.0.6/bzip2-1.0.6.tar.gz
  fi
  echo "Unpacking..."
  tar -xf bzip2-1.0.6.tar.gz >> $LOG_PATH/build.log 2>&1
  cd bzip2-1.0.6
  echo -n "Building..."
  echo "---------------------------- Build bzip2 ----------------------------" >> $LOG_PATH/build.log 2>&1
  make CFLAGS="$CFLAGS -Wno-format" PREFIX="$INSTALL_LOCATION" -s install >> $LOG_PATH/build.log 2>&1 &
  spin
  make -s clean >> $LOG_PATH/build.log 2>&1 &
  spin
  echo "done."
  echo "bzip2 Installed!"
  echo
  cd "$BUILD_LOCATION"
fi

#######################################################################

#pkg-config

if [ -e "$BIN_PATH/pkg-config" ]
then
  echo "Found pkg-config"
else
  echo "Installing pkg-config"
  if [ -e pkg-config-0.28.tar.gz ]
  then
    echo "Tarball Present..."
  else
    echo "Downloading..."
    curl -L --progress-bar -O https://pkg-config.freedesktop.org/releases/pkg-config-0.28.tar.gz
  fi
  echo "Unpacking..."
  tar xfz pkg-config-0.28.tar.gz
  cd pkg-config-0.28
  echo -n "Configuring..."
  echo "---------------------------- Configure pkg-config ----------------------------" >> $LOG_PATH/build.log 2>&1
  ./configure CFLAGS="-Os -arch x86_64" LDFLAGS="-arch x86_64" --prefix "$INSTALL_LOCATION" --with-internal-glib >> $LOG_PATH/build.log 2>&1 &
  spin
  echo "done."
  echo -n "Building..."
  echo "---------------------------- Build pkg-config ----------------------------" >> $LOG_PATH/build.log 2>&1
  #make -s RUN_FC_CACHE_TEST=false -s install >> $LOG_PATH/build.log 2>&1 &
  #spin
  make -s install >> $LOG_PATH/build.log 2>&1 &
  #>> $LOG_PATH/build.log 2>&1 &
  spin
  make -s clean >> $LOG_PATH/build.log 2>&1 &
  spin
  echo "done."
  echo "pkg-config Installed!"
  echo
  cd "$BUILD_LOCATION"
fi

#######################################################################

#expat
if [ -e "$LIB_PATH/libexpat.a" ]
then
  echo "Found expat"
else
  echo
  echo "Installing expat"
  if [ -e expat-2.2.5.tar.bz2 ]
  then
    echo "Tarball Present..."
  else
    echo "Downloading..."
    curl -L --progress-bar -O https://github.com/libexpat/libexpat/releases/download/R_2_2_5/expat-2.2.5.tar.bz2
  fi
  echo "Unpacking..."
  tar -jxf "expat-2.2.5.tar.bz2"
  cd expat-2.2.5
  echo -n "Configuring..."
  echo "---------------------------- Configure expat ----------------------------" >> $LOG_PATH/build.log 2>&1
  ./configure --disable-shared --enable-static --prefix "$INSTALL_LOCATION" >> $LOG_PATH/build.log 2>&1 &
  spin
  echo "done."
  echo -n "Building..."
  echo "---------------------------- Build expat ----------------------------" >> $LOG_PATH/build.log 2>&1
  make -s install >> $LOG_PATH/build.log 2>&1 &
  spin
  make -s clean >> $LOG_PATH/build.log 2>&1 &
  spin
  echo "done."
  echo
  echo "expat Installed!"
  echo
  cd "$BUILD_LOCATION"
fi
#######################################################################

#zlib
if [ -e "$LIB_PATH/libz.a" ]
then
  echo "Found zlib"
else
  COPTZL="-Wno-shift-negative-value"
  echo
  echo "Installing zlib"
  if [ -e zlib-1.2.11.tar.gz ]
  then
    echo "Tarball Present..."
  else
    echo "Downloading..."
    curl -L --progress-bar -O https://www.zlib.net/zlib-1.2.11.tar.gz
  fi
  echo "Unpacking..."
  tar -xf zlib-1.2.11.tar.gz
  cd zlib-1.2.11
  echo -n "Configuring..."
  echo "---------------------------- Configure zlib ----------------------------" >> $LOG_PATH/build.log 2>&1
  ./configure --static --archs="-arch i386 -arch x86_64" --prefix "$INSTALL_LOCATION" >> $LOG_PATH/build.log 2>&1 &
  spin
  echo "done."
  echo -n "Building..."
  echo "---------------------------- Build zlib ----------------------------" >> $LOG_PATH/build.log 2>&1
  make CFLAGS="$CFLAGS $COPTZL" -s install >> $LOG_PATH/build.log 2>&1 &
  spin
  make -s clean >> $LOG_PATH/build.log 2>&1 &
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
  if [ -e libpng-1.6.34.tar.xz ]
  then
    echo "Tarball Present..."
  else
    echo "Downloading..."
    curl -L --progress-bar -O http://github.com/glennrp/libpng-releases/raw/master/libpng-1.6.34.tar.xz
  fi
  echo "Unpacking..."
  tar -xf libpng-1.6.34.tar.xz
  cd libpng-1.6.34
  echo -n "Configuring..."
  echo "---------------------------- Configure libpng ----------------------------" >> $LOG_PATH/build.log 2>&1
  ./configure --disable-dependency-tracking --enable-static --disable-shared --prefix "$INSTALL_LOCATION" >> $LOG_PATH/build.log 2>&1 &
  spin
  echo "done."
  echo -n "Building..."
  echo "---------------------------- Build libpng ----------------------------" >> $LOG_PATH/build.log 2>&1
  make -s install  >> $LOG_PATH/build.log 2>&1 &
  spin
  make -s clean >> $LOG_PATH/build.log 2>&1 &
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
  if [ -e pixman-0.34.0.tar.gz ]
  then
    echo "Tarball Present..."
  else
    echo "Downloading..."
    curl -L --progress-bar -O https://cairographics.org/releases/pixman-0.34.0.tar.gz
  fi
  echo "Unpacking..."
  tar -xf pixman-0.34.0.tar.gz
  cd pixman-0.34.0
  echo -n "Configuring..."
  echo "---------------------------- Configure pixman ----------------------------" >> $LOG_PATH/build.log 2>&1
  ./configure --enable-static --disable-dependency-tracking --disable-gtk --prefix "$INSTALL_LOCATION" PKG_CONFIG="$BIN_PATH/pkg-config" PKG_CONFIG_LIBDIR="$LIB_PATH/pkgconfig" >> $LOG_PATH/build.log 2>&1 &
  spin
  echo "done."
  echo -n "Building..."
  echo "---------------------------- Build pixman ----------------------------" >> $LOG_PATH/build.log 2>&1
  make CFLAGS="-DHAVE_CONFIG_H $COPTPX $CFLAGS" -s install >> $LOG_PATH/build.log 2>&1 &
  spin
  make -s clean >> $LOG_PATH/build.log 2>&1 &
  spin
  echo "done."
  echo
  #Must remove this after build, as building without them fails
  #rm "$LIB_PATH/libpixman-1.0.dylib"
  #rm "$LIB_PATH/libpixman-1.dylib"
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
  if [ -e freetype-2.9.tar.bz2 ] || [ -e freetype-2.9.tar ]
  then
    echo "Tarball Present..."
  else
    echo "Downloading..."
    curl  --progress-bar -O --disable-epsv https://ftp.osuosl.org/pub/blfs/conglomeration/freetype/freetype-2.9.tar.bz2
  fi
  echo "Unpacking..."
  if [ ! -e freetype-2.9.tar ]
  then
    "$INSTALL_LOCATION/bin/bunzip2" "$BUILD_LOCATION/freetype-2.9.tar.bz2"
  fi
  tar -xjf freetype-2.9.tar
  cd freetype-2.9
  echo -n "Configuring..."
  echo "---------------------------- Configure freetype ----------------------------" >> $LOG_PATH/build.log 2>&1
  ./configure --prefix "$INSTALL_LOCATION" --disable-shared --enable-biarch-config BZIP2_CFLAGS="-I$INCLUDE_PATH" BZIP2_LIBS="-L$LIB_PATH" PKG_CONFIG="$BIN_PATH/pkg-config" PKG_CONFIG_LIBDIR="$LIB_PATH/pkgconfig" >> $LOG_PATH/build.log 2>&1 &
  spin
  echo "done."
  echo -n "Building..."
  echo "---------------------------- Build freetype ----------------------------" >> $LOG_PATH/build.log 2>&1
  make -s install >> $LOG_PATH/build.log 2>&1 &
  spin
  make -s clean >> $LOG_PATH/build.log 2>&1 &
  spin
  echo "done."
  echo
  echo "freetype Installed!"
  echo
  cd "$BUILD_LOCATION"
fi

#######################################################################

#fontconfig

if [ -e "$LIB_PATH/libfontconfig.a" ]
then
  echo "Found fontconfig"
else
  COPTFC="-Wno-macro-redefined -Wno-unused-command-line-argument -Wno-non-literal-null-conversion -Wno-pointer-bool-conversion -Wno-unused-function"
  echo
  echo "Installing fontconfig"
  if [ -e fontconfig-2.12.6.tar.bz2 ] || [ -e fontconfig-2.12.6.tar ]
  then
    echo "Tarball Present..."
  else
    echo "Downloading..."
    curl -L --progress-bar -O https://www.freedesktop.org/software/fontconfig/release/fontconfig-2.12.6.tar.bz2
  fi
  echo "Unpacking..."
  if [ ! -e fontconfig-2.12.6.tar ]
  then
    "$INSTALL_LOCATION/bin/bunzip2" "$BUILD_LOCATION/fontconfig-2.12.6.tar.bz2"
  fi
  tar -xf fontconfig-2.12.6.tar
  cd fontconfig-2.12.6
  echo -n "Configuring..."
  echo "---------------------------- Configure fontconfig ----------------------------" >> $LOG_PATH/build.log 2>&1
  ./configure --disable-dependency-tracking --disable-shared --enable-static --silent CFLAGS="$CFLAGS $COPTFC" --prefix "$INSTALL_LOCATION" LDFLAGS="$LDFLAGS -L$LIB_PATH" LIBS="-lbz2" PKG_CONFIG="$BIN_PATH/pkg-config" PKG_CONFIG_LIBDIR="$LIB_PATH/pkgconfig" >> $LOG_PATH/build.log 2>&1 &
  spin
  echo "done."
  echo -n "Building..."
  echo "---------------------------- Build fontconfig ----------------------------" >> $LOG_PATH/build.log 2>&1
  make -s RUN_FC_CACHE_TEST=false -s install >> $LOG_PATH/build.log 2>&1 &
  spin
  make -s clean >> $LOG_PATH/build.log 2>&1 2>&1 &
  spin
  echo "done."
  echo "fontconfig Installed!"
  echo
  cd "$BUILD_LOCATION"
fi

#######################################################################

#cairo
if [ -e "$LIB_PATH/libcairo.a" ]
then
  echo "Found cairo"
else
  COPTCR="-Wno-logical-not-parentheses -Wno-parentheses-equality -Wno-enum-conversion -Wno-unused-command-line-argument -Wno-unused-function -Wno-unused-variable -Wno-unused-local-typedef -Wno-tautological-constant-out-of-range-compare -Wno-absolute-value -Wno-literal-conversion"
  echo
  echo "Installing cairo"
  if [ -e cairo-1.14.12.tar.xz ]
  then
    echo "Tarball Present..."
  else
    echo "Downloading..."
    curl -L --progress-bar -O https://cairographics.org/releases/cairo-1.14.12.tar.xz
  fi
  echo "Unpacking..."
  tar -xf cairo-1.14.12.tar.xz
  cd cairo-1.14.12
  echo -n "Configuring..."
  echo "---------------------------- Configure cairo ----------------------------" >> $LOG_PATH/build.log 2>&1
  ./configure --disable-dependency-tracking --enable-svg=yes --enable-quartz-image=yes --disable-shared --enable-static --silent CFLAGS="$CFLAGS $COPTCR" --prefix "$INSTALL_LOCATION" PKG_CONFIG="$BIN_PATH/pkg-config" PKG_CONFIG_LIBDIR="$LIB_PATH/pkgconfig" LDFLAGS="$LDFLAGS -framework CoreFoundation -framework CoreGraphics -framework CoreText" >> $LOG_PATH/build.log 2>&1 &
  spin
  echo "done."
  echo -n "Building..."
  echo "---------------------------- Build cairo ----------------------------" >> $LOG_PATH/build.log 2>&1
  make -s install >> $LOG_PATH/build.log 2>&1 &
  spin
  make -s clean >> $LOG_PATH/build.log 2>&1 &
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
file "$LIB_PATH/libbz2.a"
file "$LIB_PATH/libexpat.a"
file "$LIB_PATH/libz.a"
file "$LIB_PATH/libpixman-1.a"
file "$LIB_PATH/libpng16.a"
file "$LIB_PATH/libfreetype.a"
file "$LIB_PATH/libfontconfig.a"
file "$LIB_PATH/libcairo.a"
exit
