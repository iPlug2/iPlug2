#!/bin/sh

#comment if you allready have depot tools!
git clone 'https://chromium.googlesource.com/chromium/tools/depot_tools.git' ../../tmp/depot_tools
export PATH="${PWD}/../../tmp/depot_tools:${PATH}"

cd ../Build/src/skia
python tools/git-sync-deps
bin/gn gen ../../tmp/skia/Release --args='
is_official_build=true
skia_use_metal=true
skia_use_icu=false
skia_enable_pdf=false
skia_enable_gpu=true
skia_use_dng_sdk=false
skia_use_expat=true
skia_use_libwebp=false
skia_use_piex=false
skia_use_sfntly=false
skia_use_system_libpng=false
skia_use_system_libjpeg_turbo=false
extra_cflags=["-Wno-error"]
extra_cflags_c=["-Wno-error"]
'
ninja -C ../../tmp/skia/Release
mv ../../tmp/skia/Release/libskia.a ../../mac/lib