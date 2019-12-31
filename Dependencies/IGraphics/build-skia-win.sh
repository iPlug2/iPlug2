
DEPOT_TOOLS_PATH=../Build/tmp/depot_tools

if [ ! -d $DEPOT_TOOLS_PATH ]; then
  git clone 'https://chromium.googlesource.com/chromium/tools/depot_tools.git' $DEPOT_TOOLS_PATH
  export PATH="${PWD}/$DEPOT_TOOLS_PATH:${PATH}"
fi

if [ "$1" = "Release" ]; then
  cd ../Build/src/skia
  python tools/git-sync-deps
  ./bin/gn gen ../../tmp/skia/Release --args='
  is_official_build = true
  is_debug = false
  target_cpu = "x64"
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
  skia_use_icu = true
  skia_use_sfntly = false
  skia_enable_skottie = true
  skia_enable_pdf = false
  skia_enable_particles = true
  skia_enable_gpu = true
  skia_enable_skparagraph = true
  skia_enable_sksl_interpreter = true
  cflags = [ "/MT" ]
  cc = "clang"
  cxx = "clang++"
  clang_win = "C:\Program Files\LLVM"
  '

  ninja -C ../../tmp/skia/Release

  mv ../../tmp/skia/Release/skia.lib ../../win/x64/Release
  mv ../../tmp/skia/Release/skottie.lib ../../win/x64/Release
  mv ../../tmp/skia/Release/sksg.lib ../../win/x64/Release
  mv ../../tmp/skia/Release/skshaper.lib ../../win/x64/Release
  mv ../../tmp/skia/Release/skparagraph.lib ../../win/x64/Release

elif [ "$1" = "Debug" ]; then
  cd ../Build/src/skia
  python tools/git-sync-deps
  ./bin/gn gen ../../tmp/skia/Debug --args='
  is_official_build = true
  is_debug = false
  target_cpu = "x64"
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
  skia_use_icu = true
  skia_use_sfntly = false
  skia_enable_skottie = true
  skia_enable_pdf = false
  skia_enable_particles = true
  skia_enable_gpu = true
  skia_enable_skparagraph = true
  skia_enable_sksl_interpreter = true
  extra_cflags = [ "/MTd" ]
  cc = "clang"
  cxx = "clang++"
  clang_win = "C:\Program Files\LLVM"
  '

  ninja -C ../../tmp/skia/Debug

  mv ../../tmp/skia/Debug/skia.lib ../../win/x64/Debug
  mv ../../tmp/skia/Debug/skottie.lib ../../win/x64/Debug
  mv ../../tmp/skia/Debug/sksg.lib ../../win/x64/Debug
  mv ../../tmp/skia/Debug/skshaper.lib ../../win/x64/Debug
  mv ../../tmp/skia/Debug/skparagraph.lib ../../win/x64/Debug

else
  echo error - call this script with either "Debug" or "Release" as the argument
fi


