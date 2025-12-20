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
LINUX_LIB_DIR="$BASE_DIR/linux/lib"

LIBS=(
  "libskia.a"
  "libskottie.a"
  "libskshaper.a"
  "libsksg.a"
  "libskparagraph.a"
  "libskunicode_icu.a"
  "libskunicode_core.a"
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
  # Continue on error - some deps may fail but essential ones may already be present
  python3 tools/git-sync-deps || echo "Warning: Some deps failed to sync, continuing anyway..."

  # Ensure GN is available
  if [ ! -f "./bin/gn" ]; then
    echo "Fetching GN..."
    ./bin/fetch-gn
  fi
}

generate_build_files() {
  local output_dir="$TMP_DIR/linux_x64"

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
    skia_use_icu = true
    skia_use_dawn = false
    skia_use_fontconfig = true
    skia_use_freetype = true
    skia_enable_skottie = true
    skia_enable_svg = true
    skia_enable_pdf = false
    skia_enable_gpu = false
    skia_use_gl = false
    skia_enable_graphite = false
    skia_enable_skparagraph = true
    skia_enable_skunicode = true
    cc = \"clang\"
    cxx = \"clang++\"
    target_os = \"linux\"
    target_cpu = \"x64\"
    extra_cflags_c = [\"-Wno-error\"]
  "
}

build_skia() {
  local output_dir="$TMP_DIR/linux_x64"

  ninja -C "$output_dir"

  if [ $? -ne 0 ]; then
    echo "Error: Build failed"
    exit 1
  fi
}

move_libs() {
  local src_dir="$TMP_DIR/linux_x64"

  mkdir -p "$LINUX_LIB_DIR"

  for lib in "${LIBS[@]}"; do
    if [ -f "$src_dir/$lib" ]; then
      mv "$src_dir/$lib" "$LINUX_LIB_DIR"
      echo "Moved $lib to $LINUX_LIB_DIR"
    else
      echo "Warning: $lib not found in $src_dir"
    fi
  done
}

main() {
  setup_depot_tools
  sync_deps
  generate_build_files
  build_skia
  move_libs

  echo "Build completed successfully"
}

main
