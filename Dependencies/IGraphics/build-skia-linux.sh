#!/bin/sh

DEPOT_TOOLS_PATH=../Build/tmp/depot_tools
BUILD_DIR=../../tmp/skia/linux_x64
DEST_DIR=../../linux/lib

if [ ! -d $DEPOT_TOOLS_PATH ]; then
  git clone --depth=2 'https://chromium.googlesource.com/chromium/tools/depot_tools.git' $DEPOT_TOOLS_PATH
fi

is_py2() {
  return $([ "$(python -c 'import sys; print(sys.version_info < (3, 0))')" = "True" ])
}

# Skia requires building with Python 2, so if the default is python 3, symlink.

if ! is_py2; then
  if [ -z "$(which python2)" ]; then
    echo "No python2 found!" && exit 1
  fi

  _cwd="$(pwd)"
  cd $DEPOT_TOOLS_PATH
  ln -s "$(which python2)" python
  cd "$_cwd"
fi

export PATH="${PWD}/$DEPOT_TOOLS_PATH:${PATH}"
# bash on my machine caches executable locations, so clear the cache
if [ "$SHELL" = "bash" ]; then
  hash -r
fi

cd ../Build/src/skia
python3 tools/git-sync-deps
./bin/gn gen ${BUILD_DIR} --args='
is_official_build = true
skia_use_system_libjpeg_turbo = false
skia_use_system_libpng = false
skia_use_system_zlib = false
skia_use_system_expat = false
skia_use_system_icu = false
skia_use_system_harfbuzz = false
skia_use_libwebp_decode = false
skia_use_libwebp_encode = false
skia_use_xps = false
skia_use_dng_sdk = false
skia_use_expat = true
skia_use_metal = false
skia_use_icu = true
skia_use_sfntly = false
skia_enable_svg = true
skia_enable_skottie = true
skia_enable_pdf = false
skia_enable_particles = true
skia_enable_gpu = true
skia_enable_skparagraph = true
skia_enable_sksl_interpreter = true
cc = "gcc"
cxx = "g++"
target_os = "linux"
target_cpu = "x64"
extra_cflags = []
extra_cflags_c = ["-Wno-error"]
'
ninja -C ${BUILD_DIR}

if [ ! -d ${DEST_DIR} ]; then
  mkdir -p ${DEST_DIR}
fi

mv ${BUILD_DIR}/libskia.a ${DEST_DIR}
mv ${BUILD_DIR}/libskottie.a ${DEST_DIR}
mv ${BUILD_DIR}/libskshaper.a ${DEST_DIR}
mv ${BUILD_DIR}/libsksg.a ${DEST_DIR}
mv ${BUILD_DIR}/libskparagraph.a ${DEST_DIR}
mv ${BUILD_DIR}/libsvg.a ${DEST_DIR}