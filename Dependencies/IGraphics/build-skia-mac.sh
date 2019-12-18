#!/bin/sh

DEPOT_TOOLS_PATH=../Build/tmp/depot_tools

if [ ! -d $DEPOT_TOOLS_PATH ]; then
  git clone 'https://chromium.googlesource.com/chromium/tools/depot_tools.git' $DEPOT_TOOLS_PATH
  export PATH="${PWD}/$DEPOT_TOOLS_PATH:${PATH}"
fi

cd ../Build/src/skia
python tools/git-sync-deps
./bin/gn gen ../../tmp/skia/macOS_x64 --args='
is_official_build = true
skia_use_system_libjpeg_turbo = false
skia_use_system_libpng = false
skia_use_system_zlib = false
skia_use_system_expat = false
skia_use_libwebp = false
skia_use_xps = false
skia_use_dng_sdk = false
skia_use_expat = true
skia_use_metal = true
skia_use_icu = false
skia_use_sfntly = false
skia_enable_skottie = false
skia_enable_pdf = false
skia_enable_particles = true
skia_enable_gpu = true
cc = "clang"
cxx = "clang++"
target_os = "mac"
target_cpu = "x64"
extra_cflags = ["-mmacosx-version-min=10.9"]
extra_cflags_c = ["-Wno-error"]
'
ninja -C ../../tmp/skia/macOS_x64
mv ../../tmp/skia/macOS_x64/libskia.a ../../mac/lib