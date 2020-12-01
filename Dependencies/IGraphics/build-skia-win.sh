
DEPOT_TOOLS_PATH=../Build/tmp/depot_tools
ERROR_STR="error - call this script with either Debug or Release as the first argument, and x64 or Win32 as the second argument"

if [ "$#" -eq 2 ]; then

  if [ "$1" = "Release" ]; then
    CONFIG_STR="Release"
  elif [ "$1" = "Debug" ]; then
    CONFIG_STR="Debug"
  else
    echo $ERROR_STR
    exit 1
  fi

  if [ "$2" = "x64" ]; then
    CPU_STR="x64"
    DIR_ARCH_STR="x64"
  elif [ "$2" = "Win32" ]; then
    CPU_STR="x86"
    DIR_ARCH_STR="Win32"
  else
    echo $ERROR_STR
    exit 1
  fi

  if [ ! -d $DEPOT_TOOLS_PATH ]; then
    echo "checking out Depot Tools..."
    git clone 'https://chromium.googlesource.com/chromium/tools/depot_tools.git' $DEPOT_TOOLS_PATH
    export PATH="${PWD}/$DEPOT_TOOLS_PATH:${PATH}"
  fi

  cd ../Build/src/skia

  echo "Syncing Deps..."
  python tools/git-sync-deps

  ./bin/gn gen ../../tmp/skia/$DIR_ARCH_STR/$CONFIG_STR --args='
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
  clang_win = "C:\Program Files\LLVM"
  '

  if [ $CONFIG_STR = "Debug" ]; then
    echo 'extra_cflags = [ "/MTd" ]' >> ../../tmp/skia/$DIR_ARCH_STR/$CONFIG_STR/args.gn

    echo 'is_debug = false' >> ../../tmp/skia/$DIR_ARCH_STR/$CONFIG_STR/args.gn
    echo 'is_official_build = true' >> ../../tmp/skia/$DIR_ARCH_STR/$CONFIG_STR/args.gn

    # disabled due to massive binaries
    # echo 'is_debug = true' >> ../../tmp/skia/$DIR_ARCH_STR/$CONFIG_STR/args.gn
    # echo 'is_official_build = false' >> ../../tmp/skia/$DIR_ARCH_STR/$CONFIG_STR/args.gn
  else
    echo 'extra_cflags = [ "/MT" ]' >> ../../tmp/skia/$DIR_ARCH_STR/$CONFIG_STR/args.gn
    echo 'is_debug = false' >> ../../tmp/skia/$DIR_ARCH_STR/$CONFIG_STR/args.gn
    echo 'is_official_build = true' >> ../../tmp/skia/$DIR_ARCH_STR/$CONFIG_STR/args.gn
  fi

  if [ $DIR_ARCH_STR = "Win32" ]; then
    echo 'target_cpu = "x86"' >> ../../tmp/skia/$DIR_ARCH_STR/$CONFIG_STR/args.gn
  else
    echo 'target_cpu = "x64"' >> ../../tmp/skia/$DIR_ARCH_STR/$CONFIG_STR/args.gn
  fi

  ./bin/gn gen ../../tmp/skia/$DIR_ARCH_STR/$CONFIG_STR

  ninja -C ../../tmp/skia/$DIR_ARCH_STR/$CONFIG_STR

  mv ../../tmp/skia/$DIR_ARCH_STR/$CONFIG_STR/skia.lib ../../win/$DIR_ARCH_STR/$CONFIG_STR
  mv ../../tmp/skia/$DIR_ARCH_STR/$CONFIG_STR/skottie.lib ../../win/$DIR_ARCH_STR/$CONFIG_STR
  mv ../../tmp/skia/$DIR_ARCH_STR/$CONFIG_STR/sksg.lib ../../win/$DIR_ARCH_STR/$CONFIG_STR
  mv ../../tmp/skia/$DIR_ARCH_STR/$CONFIG_STR/skshaper.lib ../../win/$DIR_ARCH_STR/$CONFIG_STR
  mv ../../tmp/skia/$DIR_ARCH_STR/$CONFIG_STR/skparagraph.lib ../../win/$DIR_ARCH_STR/$CONFIG_STR
  mv ../../tmp/skia/$DIR_ARCH_STR/$CONFIG_STR/svg.lib ../../win/$DIR_ARCH_STR/$CONFIG_STR

else
  echo $ERROR_STR
fi

