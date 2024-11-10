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
WIN_LIB_DIR="$BASE_DIR/win"
WIN_BIN_DIR="$BASE_DIR/win/bin"

ERROR_MSG="Error: Please provide 'Debug' or 'Release' as the first argument, and 'x64' or 'Win32' as the second argument."

LIBS=(
  "skia.lib"
  "skottie.lib"
  "sksg.lib"
  "skshaper.lib"
  "skparagraph.lib"
  "skunicode_icu.lib"
  "skunicode_core.lib"
  "svg.lib"
)

ICU_DATA="icudtl.dat"

setup_depot_tools() {
  if [ ! -d "$DEPOT_TOOLS_PATH" ]; then
    echo "Checking out Depot Tools..."
    git clone 'https://chromium.googlesource.com/chromium/tools/depot_tools.git' "$DEPOT_TOOLS_PATH"
  fi
  export PATH="$DEPOT_TOOLS_PATH:${PATH}"
}

sync_deps() {
  cd "$SKIA_SRC_DIR"
  echo "Syncing Deps..."
  python tools/git-sync-deps
}

generate_build_files() {
  local config=$1
  local arch=$2
  local output_dir="$TMP_DIR/$arch/$config"

  local extra_args=""
  if [ "$config" = "Debug" ]; then
    extra_args="
      extra_cflags = [ \"/MTd\" ]
    "
  else
    extra_args="
      extra_cflags = [ \"/MT\" ]
    "
  fi

  if [ "$arch" = "Win32" ]; then
    extra_args="$extra_args
      target_cpu = \"x86\"
    "
  else
    extra_args="$extra_args
      target_cpu = \"x64\"
    "
  fi

  ./bin/gn gen "$output_dir" --args="
    is_debug = false
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
    skia_use_icu = true
    skia_use_gl = true
    skia_use_dawn = false
    skia_use_direct3d = false
    skia_enable_svg = true
    skia_enable_skottie = true
    skia_enable_pdf = false
    skia_enable_gpu = true
    skia_enable_graphite = true
    skia_enable_skparagraph = true
    cc = \"clang\"
    cxx = \"clang++\"
    clang_win = \"C:\\Program Files\\LLVM\"
    $extra_args
  "
}

build_skia() {
  local config=$1
  local arch=$2
  local output_dir="$TMP_DIR/$arch/$config"
  
  ninja -C "$output_dir"
  
  if [ $? -ne 0 ]; then
    echo "Error: Build failed for $arch $config"
    exit 1
  fi
}

move_libs() {
  local config=$1
  local arch=$2
  local src_dir="$TMP_DIR/$arch/$config"
  local dest_dir="$WIN_LIB_DIR/$arch/$config"
  
  mkdir -p "$dest_dir"
  
  for lib in "${LIBS[@]}"; do
    if [ -f "$src_dir/$lib" ]; then
      mv "$src_dir/$lib" "$dest_dir"
      echo "Moved $lib to $dest_dir"
    else
      echo "Warning: $lib not found in $src_dir"
    fi
  done

  if [ "$config" = "Release" ] && [ "$arch" = "x64" ]; then
    if [ -f "$src_dir/$ICU_DATA" ]; then
      mkdir -p "$WIN_BIN_DIR"
      cp "$src_dir/$ICU_DATA" "$WIN_BIN_DIR"
      echo "Copied $ICU_DATA to $WIN_BIN_DIR"
    else
      echo "Warning: $ICU_DATA not found in $src_dir"
    fi
  fi
}

main() {
  if [ "$#" -ne 2 ]; then
    echo "$ERROR_MSG"
    exit 1
  fi

  local config=$1
  local arch=$2

  case "$config" in
    "Debug"|"Release") ;;
    *) echo "$ERROR_MSG"; exit 1 ;;
  esac

  case "$arch" in
    "x64"|"Win32") ;;
    *) echo "$ERROR_MSG"; exit 1 ;;
  esac

  setup_depot_tools
  sync_deps
  generate_build_files "$config" "$arch"
  build_skia "$config" "$arch"
  move_libs "$config" "$arch"
}

main "$@"
