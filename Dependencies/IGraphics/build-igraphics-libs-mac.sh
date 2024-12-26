#!/usr/bin/env bash
set -e

IGRAPHICS_DEPS_DIR="$PWD"
BUILD_DIR="$IGRAPHICS_DEPS_DIR/../Build"
SRC_DIR="$BUILD_DIR/src"
TMP_DIR="$BUILD_DIR/tmp"
INSTALL_DIR="$BUILD_DIR/mac"
INCLUDE_DIR="$INSTALL_DIR/include"
LIB_DIR="$INSTALL_DIR/lib"
BIN_DIR="$INSTALL_DIR/bin"
LOG_DIR="$BUILD_DIR"
LOG_NAME="build-mac.log"
FREETYPE_OPTIONS="--disable-shared --without-zlib --without-png --without-bzip2 --without-harfbuzz --without-brotli"
DEPLOYMENT_TARGET=10.13

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

echo
echo "###################################################################################"
echo
echo "     This script will build libraries required for IGraphics on macOS,"
echo "     please relax and have a cup of tea, it'll take a while..."
echo
echo "###################################################################################"
echo

if [ ! -d "$BUILD_DIR" ]; then mkdir "$BUILD_DIR"; fi
if [ ! -d "$TMP_DIR" ]; then mkdir "$TMP_DIR"; fi
if [ ! -d "$SRC_DIR" ]; then mkdir "$SRC_DIR"; fi
if [ ! -d "$INSTALL_DIR" ]; then mkdir "$INSTALL_DIR"; fi
if [ ! -d "$LIB_DIR" ]; then mkdir "$LIB_DIR"; fi
if [ ! -d "$INCLUDE_DIR" ]; then mkdir "$INCLUDE_DIR"; fi

cd "$SRC_DIR"

echo

PLATFORMS_DIR="/Applications/Xcode.app/Contents/Developer/Platforms"
SYSROOT="${PLATFORMS_DIR}/MacOSX.platform/Developer/SDKs/MacOSX.sdk"

# remove old log file if exists
if [ -e $LOG_DIR/$LOG_NAME ]
then
  rm $LOG_DIR/$LOG_NAME
else
  touch $LOG_DIR/$LOG_NAME
fi

#######################################################################

#freetype
buildFreetype()
{
  ARCH=$1
  
  if [[ $ARCH == "x86_64" ]]; then
    HOST="i386-apple-darwin"
  elif [[ $ARCH == "arm64" ]]; then
    HOST="arm-apple-darwin"
  fi

  echo -n "Configuring freetype for ${ARCH} ..."
  cd "$SRC_DIR/freetype"
  echo "---------------------------- Configure freetype ----------------------------" >> $LOG_DIR/$LOG_NAME 2>&1
  
  echo "done."
  
  export MACOSX_DEPLOYMENT_TARGET=$DEPLOYMENT_TARGET

  # TODO: for some reason these exports cause x86 build to fail
  if [[ $ARCH == "arm64" ]]; then
    export CC="$(xcrun -sdk macosx -find clang)"
    export CXX="$(xcrun -sdk macosx -find clang++)"
    export CPP="$CC -E"
    COMMON_CFLAGS="-arch $ARCH \
                  -Os \
                  -isysroot $SYSROOT \
                  -I$SYSROOT/usr/include/libxml2"
    #-fembed-bitcode
    export CFLAGS="$COMMON_CFLAGS"
    export CXXFLAGS="$COMMON_CFLAGS"
    export AR=$(xcrun -sdk macosx -find ar)
    export RANLIB=$(xcrun -sdk macosx -find ranlib)
    # export CPPFLAGS="-arch $ARCH -isysroot $SYSROOT"
    export LDFLAGS="-arch $ARCH -isysroot $SYSROOT"
  fi
  
  #PKG_CONFIG="$BIN_DIR/pkg-config" PKG_CONFIG_LIBDIR="$LIB_DIR/pkgconfig"
  ./configure --prefix "$TMP_DIR"/freetype/mac/$ARCH $FREETYPE_OPTIONS --host=$HOST
  # TODO: trap not working when configure fails
  #>> $LOG_DIR/$LOG_NAME 2>&1 &
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
}

buildFreetype x86_64
buildFreetype arm64

LIPO=$(xcrun -sdk macosx -find lipo)
$LIPO -create "$TMP_DIR"/freetype/mac/x86_64/lib/libfreetype.a "$TMP_DIR"/freetype/mac/arm64/lib/libfreetype.a -output $LIB_DIR/libfreetype.a
mv $TMP_DIR/freetype/mac/x86_64/include/freetype2 $INCLUDE_DIR

