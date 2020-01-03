#!/bin/sh

DEPOT_TOOLS_PATH=../Build/tmp/depot_tools

if [ ! -d $DEPOT_TOOLS_PATH ]; then
  git clone 'https://chromium.googlesource.com/chromium/tools/depot_tools.git' $DEPOT_TOOLS_PATH
  export PATH="${PWD}/$DEPOT_TOOLS_PATH:${PATH}"
fi

cd ../Build/src/skia
python tools/git-sync-deps
./bin/gn gen ../../tmp/skia/iOS_arm64 --args='
is_official_build = true
skia_use_system_libjpeg_turbo = false
skia_use_system_libpng = false
skia_use_system_zlib = false
skia_use_system_expat = false
skia_use_system_icu = false
skia_use_system_harfbuzz = false
skia_use_libwebp = false
skia_use_xps = false
skia_use_dng_sdk = false
skia_use_expat = true
skia_use_metal = true
skia_use_icu = true
skia_use_sfntly = false
skia_enable_skottie = true
skia_enable_pdf = false
skia_enable_particles = true
skia_enable_gpu = true
skia_enable_skparagraph = true
skia_enable_sksl_interpreter = true
cc = "clang"
cxx = "clang++"
target_os = "ios"
target_cpu = "arm64"
extra_cflags = ["-miphoneos-version-min=12.1",
"-I../../../src/skia/third_party/externals/expat/lib"]
extra_cflags_c = ["-Wno-error"]
'

ninja -C ../../tmp/skia/iOS_arm64

if [ ! -d ../../ios/lib/arm64 ]; then
  mkdir -p ../../ios/lib/arm64
fi

mv ../../tmp/skia/iOS_arm64/libskia.a ../../ios/lib/arm64
mv ../../tmp/skia/iOS_arm64/libskottie.a ../../ios/lib/arm64
mv ../../tmp/skia/iOS_arm64/libskshaper.a ../../ios/lib/arm64
mv ../../tmp/skia/iOS_arm64/libsksg.a ../../ios/lib/arm64
mv ../../tmp/skia/iOS_arm64/libskparagraph.a ../../ios/lib/arm64

# echo "Building for iOS simulator"

# ./bin/gn gen ../../tmp/skia/iOS_x64 --args='
# is_official_build = true
# skia_use_system_libjpeg_turbo = false
# skia_use_system_libpng = false
# skia_use_system_zlib = false
# skia_use_system_expat = false
# skia_use_system_icu = false
# skia_use_system_harfbuzz = false
# skia_use_libwebp = false
# skia_use_xps = false
# skia_use_dng_sdk = false
# skia_use_expat = true
# skia_use_metal = true
# skia_use_icu = true
# skia_use_sfntly = false
# skia_enable_skottie = true
# skia_enable_pdf = false
# skia_enable_particles = true
# skia_enable_gpu = true
# skia_enable_skparagraph = true
# cc = "clang"
# cxx = "clang++"
# target_os = "ios"
# target_cpu = "x64"
# extra_cflags = ["-mios-simulator-version-min=12.1",
# "-I../../../src/skia/third_party/externals/expat/lib"]
# extra_cflags_c = ["-Wno-error"]
# '

# ninja -C ../../tmp/skia/iOS_x64

# if [ ! -d ../../ios/lib/x64 ]; then
#   mkdir -p ../../ios/lib/x64
# fi

# mv ../../tmp/skia/iOS_x64/libskia.a ../../ios/lib/x64
# mv ../../tmp/skia/iOS_x64/libskottie.a ../../ios/lib/x64
# mv ../../tmp/skia/iOS_x64/libskshaper.a ../../ios/lib/x64
# mv ../../tmp/skia/iOS_x64/libsksg.a ../../ios/lib/x64
# mv ../../tmp/skia/iOS_x64/libskparagraph.a ../../ios/lib/x64
