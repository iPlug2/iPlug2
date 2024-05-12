#!/bin/bash

# Ensure script is run from the Dependencies/IGraphics folder
if [[ $(basename "$PWD") != "IGraphics" ]]; then
    echo "Error: This script must be run from the IGraphics folder."
    exit 1
fi

BASE_DIR="$PWD/../Build"
DEPOT_TOOLS_PATH="$BASE_DIR/tmp/depot_tools"
SKIA_SRC_DIR="$BASE_DIR/src/skia"
TMP_DIR="$BASE_DIR/tmp/skia"
IOS_LIB_DIR="$BASE_DIR/ios/lib"

ERROR_MSG="Error: Please provide either 'arm64' or 'x86_64' as a single argument."

LIBS=(
  "libskia.a"
  "libskottie.a"
  "libsksg.a"
  "libskshaper.a"
  "libskparagraph.a"
  "libskunicode_core.a"
  "libskunicode_icu.a"
  "libsvg.a"
)

setup_depot_tools() {
  if [ ! -d "$DEPOT_TOOLS_PATH" ]; then
    git clone 'https://chromium.googlesource.com/chromium/tools/depot_tools.git' "$DEPOT_TOOLS_PATH"
  fi
  export PATH="${PWD}/$DEPOT_TOOLS_PATH:${PATH}"
}

sync_deps() {
  cd "$SKIA_SRC_DIR"
  echo "Syncing Deps..."
  python3 tools/git-sync-deps
}

generate_build_files() {
  local arch=$1
  local cpu_type=$2
  local output_dir="$TMP_DIR/$arch"

  ./bin/gn gen "$output_dir" --args="
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
    skia_enable_svg = true
    skia_enable_skottie = true
    skia_enable_pdf = false
    skia_enable_gpu = true
    skia_enable_skparagraph = true
    skia_ios_use_signing = false
    cc = \"clang\"
    cxx = \"clang++\"
    target_os = \"ios\"
    target_cpu = \"$cpu_type\"
    extra_cflags = [\"-miphoneos-version-min=13\", \"-I../../../src/skia/third_party/externals/expat/lib\"]
    extra_cflags_c = [\"-Wno-error\"]
    extra_asmflags = [\"-fembed-bitcode\"]
  "
}

build_skia() {
  local arch=$1
  local output_dir="$TMP_DIR/$arch"
  
  ninja -C "$output_dir"
  
  if [ $? -ne 0 ]; then
    echo "Error: Build failed for $arch"
    exit 1
  fi
}

move_libs() {
  local arch=$1
  local src_dir="$TMP_DIR/$arch"
  local dest_dir="$IOS_LIB_DIR/$arch"
  
  mkdir -p "$dest_dir"
  
  for lib in "${LIBS[@]}"; do
    if [ -f "$src_dir/$lib" ]; then
      mv "$src_dir/$lib" "$dest_dir"
      echo "Moved $lib to $dest_dir"
    else
      echo "Warning: $lib not found in $src_dir"
    fi
  done
}

main() {
  if [ "$#" -ne 1 ]; then
    echo "$ERROR_MSG"
    exit 1
  fi

  case "$1" in
    "x86_64")
      ARCH="x86_64"
      CPU_TYPE="x64"
      ;;
    "arm64")
      ARCH="arm64"
      CPU_TYPE="arm64"
      ;;
    *)
      echo "$ERROR_MSG"
      exit 1
      ;;
  esac

  setup_depot_tools
  sync_deps
  generate_build_files "$ARCH" "$CPU_TYPE"
  build_skia "$ARCH"
  move_libs "$ARCH"

  echo "Build completed successfully for $ARCH"
}

main "$@"
