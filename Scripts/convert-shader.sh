#!/usr/bin/env bash
#
# convert-shader.sh - Convert GLSL 450 shaders to multiple backend formats
#
# Usage: convert-shader.sh <input.frag> <output_dir> [targets...]
#
# Targets: msl, msl-ios, glsl, essl, hlsl, all (default: all)
#
# The input shader should be written in GLSL 450 with a uniform block:
#
#   #version 450
#   layout(binding = 0) uniform Uniforms {
#     float uTime;
#     vec2 uResolution;
#     vec2 uMouse;
#     vec2 uMouseButtons;
#   };
#   layout(location = 0) out vec4 FragColor;
#   void main() { ... }
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IPLUG2_ROOT="$(dirname "$SCRIPT_DIR")"

# Find axslcc binary
if [[ "$OSTYPE" == "darwin"* ]]; then
  AXSLCC="$IPLUG2_ROOT/Dependencies/Build/mac/bin/axslcc"
elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" ]]; then
  AXSLCC="$IPLUG2_ROOT/Dependencies/Build/win/bin/axslcc.exe"
else
  AXSLCC="$IPLUG2_ROOT/Dependencies/Build/linux/bin/axslcc"
fi

if [[ ! -x "$AXSLCC" ]]; then
  echo "Error: axslcc not found at $AXSLCC"
  echo "Please download axslcc from https://github.com/axmolengine/axslcc/releases"
  exit 1
fi

usage() {
  echo "Usage: $0 <input.frag> <output_dir> [targets...]"
  echo ""
  echo "Targets:"
  echo "  msl      - Metal Shading Language (macOS)"
  echo "  msl-ios  - Metal Shading Language (iOS)"
  echo "  glsl     - OpenGL 3.3 GLSL"
  echo "  essl     - OpenGL ES 3.0"
  echo "  hlsl     - HLSL (Direct3D)"
  echo "  all      - All targets (default)"
  echo ""
  echo "Example:"
  echo "  $0 MyShader.frag ./shaders msl glsl"
  exit 1
}

if [[ $# -lt 2 ]]; then
  usage
fi

INPUT="$1"
OUTPUT_DIR="$2"
shift 2

# Default to all targets
if [[ $# -eq 0 ]]; then
  TARGETS="msl msl-ios glsl essl"
else
  TARGETS="$@"
fi

# Get base name without extension
BASENAME=$(basename "$INPUT" .frag)
BASENAME=$(basename "$BASENAME" .glsl)

mkdir -p "$OUTPUT_DIR"

echo "Converting $INPUT..."

for TARGET in $TARGETS; do
  case "$TARGET" in
    msl)
      echo "  -> MSL (macOS)"
      "$AXSLCC" --frag="$INPUT" -o "$OUTPUT_DIR/${BASENAME}.metal" -l msl -u
      ;;
    msl-ios)
      echo "  -> MSL (iOS)"
      "$AXSLCC" --frag="$INPUT" -o "$OUTPUT_DIR/${BASENAME}_ios.metal" -l msl --msl-ios -u
      ;;
    glsl)
      echo "  -> GLSL 3.30"
      "$AXSLCC" --frag="$INPUT" -o "$OUTPUT_DIR/${BASENAME}.glsl" -l glsl -p 330 -u
      ;;
    glsl-h)
      echo "  -> GLSL 3.30 (C header)"
      "$AXSLCC" --frag="$INPUT" -o "$OUTPUT_DIR/${BASENAME}_glsl.h" -l glsl -p 330 -u -N "k${BASENAME}GLSL"
      ;;
    essl)
      echo "  -> ESSL 3.00"
      "$AXSLCC" --frag="$INPUT" -o "$OUTPUT_DIR/${BASENAME}.essl" -l essl -p 300 -u
      ;;
    essl-h)
      echo "  -> ESSL 3.00 (C header)"
      "$AXSLCC" --frag="$INPUT" -o "$OUTPUT_DIR/${BASENAME}_essl.h" -l essl -p 300 -u -N "k${BASENAME}ESSL"
      ;;
    hlsl)
      echo "  -> HLSL"
      "$AXSLCC" --frag="$INPUT" -o "$OUTPUT_DIR/${BASENAME}.hlsl" -l hlsl -p 50 -u
      ;;
    sksl)
      echo "  -> SkSL (Skia)"
      # Convert GLSL 450 to SkSL using awk for more control
      awk '
        BEGIN { in_uniform_block = 0 }
        /^#version/ { next }
        /layout.*out.*FragColor/ { next }
        /uniform Uniforms/ { in_uniform_block = 1; next }
        /^};$/ || /^\};$/ { if (in_uniform_block) { in_uniform_block = 0; next } }
        in_uniform_block {
          # Add uniform keyword to block members, strip leading whitespace
          line = $0
          gsub(/^[[:space:]]+/, "", line)
          if (line ~ /^(float|vec[234])/) {
            gsub(/vec2/, "float2", line)
            gsub(/vec3/, "float3", line)
            gsub(/vec4/, "half4", line)
            print "uniform " line
            next
          }
        }
        {
          # Transform main function
          gsub(/void main\(\)/, "half4 main(float2 fragCoord)")
          # Transform gl_FragCoord
          gsub(/gl_FragCoord\.xy/, "fragCoord")
          # Transform output
          gsub(/FragColor = /, "return ")
          # Transform vec types
          gsub(/vec4/, "half4")
          gsub(/vec3/, "half3")
          gsub(/vec2/, "float2")
          print
        }
      ' "$INPUT" > "$OUTPUT_DIR/${BASENAME}.sksl"
      ;;
    all)
      "$0" "$INPUT" "$OUTPUT_DIR" msl msl-ios glsl essl
      ;;
    headers)
      "$0" "$INPUT" "$OUTPUT_DIR" glsl-h essl-h
      ;;
    *)
      echo "Unknown target: $TARGET"
      usage
      ;;
  esac
done

echo "Done!"
