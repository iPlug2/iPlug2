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
MAC_LIB_DIR="$BASE_DIR/mac/lib"

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
  python3 tools/git-sync-deps
}

generate_build_files() {
  local arch=$1
  local output_dir="$TMP_DIR/macOS_$arch"

  local extra_args=""
  if [ "$arch" = "arm64" ]; then
    extra_args="
      target_cpu = \"arm64\"
      extra_cflags = [\"-mmacosx-version-min=11.0\"]
      extra_asmflags = [\"-mmacosx-version-min=11.0\"]
    "
  else
    extra_args="
      target_cpu = \"x86_64\"
      # Uncomment the following line if needed:
      # extra_cflags = [\"-mmacosx-version-min=10.13\"]
    "
  fi

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
    skia_enable_skottie = true
    skia_enable_svg = true
    skia_enable_pdf = false
    skia_enable_gpu = true
    skia_use_metal = true
    skia_enable_graphite = true
    skia_enable_skparagraph = true
    skia_enable_skunicode = true
    cc = \"clang\"
    cxx = \"clang++\"
    target_os = \"mac\"
    extra_cflags_c = [\"-Wno-error\"]
    $extra_args
  "
}

build_skia() {
  local arch=$1
  local output_dir="$TMP_DIR/macOS_$arch"
  
  ninja -C "$output_dir"
  
  if [ $? -ne 0 ]; then
    echo "Error: Build failed for $arch"
    exit 1
  fi
}

move_libs() {
  local arch=$1
  local src_dir="$TMP_DIR/macOS_$arch"
  local dest_dir="$MAC_LIB_DIR"_"$arch"
  
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

create_universal_libs() {
  echo 'Creating universal files...'
  mkdir -p "$MAC_LIB_DIR"
  
  for lib in "${LIBS[@]}"; do
    if [ -f "$MAC_LIB_DIR/$lib" ]; then
      rm "$MAC_LIB_DIR/$lib"
    fi
    xcrun lipo -create "$MAC_LIB_DIR"_*/"$lib" -output "$MAC_LIB_DIR/$lib"
    echo "Created universal file: $lib"
  done
}

cleanup() {
  rm -rf "$MAC_LIB_DIR"_arm64 "$MAC_LIB_DIR"_x86_64
  echo "Cleaned up temporary directories"
}

main() {
  setup_depot_tools
  sync_deps
  
  for arch in "x86_64" "arm64"; do
    generate_build_files "$arch"
    build_skia "$arch"
    move_libs "$arch"
  done
  
  create_universal_libs
  cleanup

  echo "Build completed successfully"
}

main
