#!/usr/bin/env bash
set -euo pipefail

# Build the Skia revision used by iPlug2 for the Linux GL3 backend.
# Dependencies are installed only below Dependencies/Build/linux.
root="$(cd "$(dirname "$0")/.." && pwd)"
install_dir="${IPLUG_SKIA_LINUX_DIR:-$root/Build/linux}"
src="$install_dir/src/skia"
out="$install_dir/out"
revision=b988efa06f8aa3bfeaa18c5b8c716ff244ab43cf # chrome/m130
libs=(skia svg skparagraph skshaper skunicode_core skunicode_icu)

for tool in git python3 ninja pkg-config; do
  command -v "$tool" >/dev/null || { echo "Missing build tool: $tool" >&2; exit 1; }
done
pkg-config --exists fontconfig expat freetype2 || {
  echo "Install fontconfig, expat and freetype development packages first." >&2
  exit 1
}

mkdir -p "$install_dir/src"
if [[ ! -d "$src/.git" ]]; then
  git clone --depth 1 --branch chrome/m130 https://github.com/google/skia.git "$src"
fi
if [[ "$(git -C "$src" rev-parse HEAD)" != "$revision" ]]; then
  echo "Expected Skia $revision (chrome/m130) at $src" >&2
  exit 1
fi

(cd "$src" && python3 tools/git-sync-deps)
(cd "$src" && ./bin/gn gen "$out" --args='
  is_official_build=true
  is_debug=false
  skia_enable_gpu=true
  skia_enable_graphite=false
  skia_enable_skparagraph=true
  skia_enable_svg=true
  skia_use_gl=true
  skia_use_vulkan=false
  skia_use_system_icu=false
  skia_use_system_harfbuzz=false
  skia_use_system_libpng=false
  skia_use_system_zlib=false
  skia_use_system_libjpeg_turbo=false
  skia_use_libwebp_decode=false
  skia_use_libwebp_encode=false
  skia_use_perfetto=false
')
ninja -C "$out" -j "${SKIA_JOBS:-3}" "${libs[@]}"
mkdir -p "$install_dir/lib"
for lib in "${libs[@]}"; do
  cp "$out/lib${lib}.a" "$install_dir/lib/lib${lib}.a"
done
echo "Linux Skia installed in $install_dir"