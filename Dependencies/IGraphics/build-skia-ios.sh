#!/bin/sh

DEPOT_TOOLS_PATH=../Build/tmp/depot_tools
ERROR_STR="error - call this script with either arm64 or x64 as a single argument"

if [ "$#" -eq 1 ]; then

  if [ "$1" = "x64" ]; then
    DIR_ARCH_STR="x86_64"
  elif [ "$1" = "arm64" ]; then
    DIR_ARCH_STR="arm64"
  else
    echo $ERROR_STR
    exit 1
  fi

  if [ ! -d $DEPOT_TOOLS_PATH ]; then
    git clone 'https://chromium.googlesource.com/chromium/tools/depot_tools.git' $DEPOT_TOOLS_PATH
  fi

  export PATH="${PWD}/$DEPOT_TOOLS_PATH:${PATH}"

  cd ../Build/src/skia

  echo "Syncing Deps..."
  python tools/git-sync-deps
  
  ./bin/gn gen ../../tmp/skia/ios/$DIR_ARCH_STR --args='
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
  skia_use_metal = true
  skia_use_icu = true
  skia_use_sfntly = false
  skia_enable_svg = true
  skia_enable_skottie = true
  skia_enable_pdf = false
  skia_enable_particles = true
  skia_enable_gpu = true
  skia_enable_skparagraph = true
  skia_enable_sksl_interpreter = true
  cc = "clang"
  cxx = "clang++"
  target_os = "ios"
  extra_cflags = ["-miphoneos-version-min=13",
  "-I../../../src/skia/third_party/externals/expat/lib",
  "-fembed-bitcode"]
  extra_cflags_c = ["-Wno-error"]
  extra_asmflags = ["-fembed-bitcode"]
  '

  if [ $DIR_ARCH_STR = "arm64" ]; then
    echo 'target_cpu = "arm64"' >> ../../tmp/skia/ios/$DIR_ARCH_STR/args.gn
  else
    echo 'target_cpu = "x64"' >> ../../tmp/skia/ios/$DIR_ARCH_STR/args.gn
  fi

  ./bin/gn gen ../../tmp/skia/ios/$DIR_ARCH_STR

  ninja -C ../../tmp/skia/ios/$DIR_ARCH_STR

  if [ ! -d ../../ios/lib/$DIR_ARCH_STR ]; then
    mkdir -p ../../ios/lib/$DIR_ARCH_STR
  fi

  mv ../../tmp/skia/ios/$DIR_ARCH_STR/libskia.a ../../ios/lib/$DIR_ARCH_STR
  mv ../../tmp/skia/ios/$DIR_ARCH_STR/libskottie.a ../../ios/lib/$DIR_ARCH_STR
  mv ../../tmp/skia/ios/$DIR_ARCH_STR/libsksg.a ../../ios/lib/$DIR_ARCH_STR
  mv ../../tmp/skia/ios/$DIR_ARCH_STR/libskshaper.a ../../ios/lib/$DIR_ARCH_STR
  mv ../../tmp/skia/ios/$DIR_ARCH_STR/libskparagraph.a ../../ios/lib/$DIR_ARCH_STR
  mv ../../tmp/skia/ios/$DIR_ARCH_STR/libsvg.a ../../ios/lib/$DIR_ARCH_STR

else
  echo $ERROR_STR
fi