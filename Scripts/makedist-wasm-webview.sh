#!/bin/bash

# makedist-wasm-webview.sh builds a WebView UI Web version of an iPlug2
# project using the split wasm DSP/UI approach. Per-project wrappers set
# IPLUG_WASM_PROJECT_ROOT and IPLUG_WASM_IPLUG2_ROOT before invoking this file.
#
# Arguments:
# 1st argument : either "on" or "off" - whether to launch emrun after compilation
# 2nd argument : browser - "chrome", "safari", or "firefox"

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
PROJECT_ROOT="${IPLUG_WASM_PROJECT_ROOT:-$SCRIPT_DIR/..}"
IPLUG2_ROOT="${IPLUG_WASM_IPLUG2_ROOT:-$SCRIPT_DIR/..}"
PROJECT_ROOT="$( cd "$PROJECT_ROOT" >/dev/null 2>&1 && pwd )"
IPLUG2_ROOT="$( cd "$IPLUG2_ROOT" >/dev/null 2>&1 && pwd )"

PROJECT_NAME="$( basename "$PROJECT_ROOT" )"

# PROJECT_NAME is interpolated into sed substitutions below, so reject any
# characters that could corrupt the replacement (anything but alphanumerics, - and _).
if ! [[ "$PROJECT_NAME" =~ ^[A-Za-z0-9_-]+$ ]]; then
  echo "ERROR: project directory name '$PROJECT_NAME' contains unsupported characters (allowed: A-Z a-z 0-9 - _)" >&2
  exit 1
fi

PROJECT_NAME_LC=$(echo "$PROJECT_NAME" | tr '[:upper:]' '[:lower:]')
EMRUN_BROWSER=chrome
LAUNCH_EMRUN=1

# Parallel make jobs (Emscripten single-threaded builds are slow).
MAKE_JOBS=$(sysctl -n hw.logicalcpu 2>/dev/null || nproc 2>/dev/null || echo 4)

cd "$PROJECT_ROOT"

if [ "$1" = "off" ]; then
  LAUNCH_EMRUN=0
fi

if [ "$#" -ge 2 ]; then
  EMRUN_BROWSER=${2}
fi

# Clean/create build directory.
if [ -d build-web-wasm/.git ]; then
  # If there's a git repo, only trash generated folders/files.
  rm -rf build-web-wasm/scripts build-web-wasm/styles build-web-wasm/web
  rm -f build-web-wasm/index.html build-web-wasm/serve.py
else
  rm -rf build-web-wasm
  mkdir build-web-wasm
fi

mkdir -p build-web-wasm/scripts
mkdir -p build-web-wasm/styles
mkdir -p build-web-wasm/web

echo "============================================================"
echo "STAGING WEBVIEW RESOURCES"
echo "============================================================"

if [ -d resources/web ]; then
  cp -R resources/web/. build-web-wasm/web/
else
  echo "WARNING: resources/web does not exist for $PROJECT_NAME"
fi

echo ""
echo "============================================================"
echo "BUILDING DSP WASM MODULE (AudioWorklet)"
echo "============================================================"

cd "$PROJECT_ROOT/projects"
emmake make -j"$MAKE_JOBS" --makefile "$PROJECT_NAME-wasm-dsp.mk"

# Wrap DSP module for AudioWorklet scope.
cd "$PROJECT_ROOT/build-web-wasm/scripts"
WORKLET_SHIM="$IPLUG2_ROOT/IPlug/WEB/TemplateWasm/scripts/worklet-scope-shim.js"
if [ ! -f "$WORKLET_SHIM" ]; then
  echo "ERROR: AudioWorklet scope shim not found at $WORKLET_SHIM" >&2
  exit 1
fi
cat "$WORKLET_SHIM" "$PROJECT_NAME-dsp.js" > "$PROJECT_NAME-dsp.tmp.js"
mv "$PROJECT_NAME-dsp.tmp.js" "$PROJECT_NAME-dsp.js"

cd "$PROJECT_ROOT/projects"

echo ""
echo "============================================================"
echo "BUILDING WEBVIEW UI WASM MODULE"
echo "============================================================"

emmake make -j"$MAKE_JOBS" --makefile "$PROJECT_NAME-wasm-ui.mk"

cd "$PROJECT_ROOT/build-web-wasm"

echo ""
echo "============================================================"
echo "GENERATING JAVASCRIPT BUNDLE"
echo "============================================================"

MAXNINPUTS=$(python3 "$IPLUG2_ROOT/Scripts/parse_iostr.py" "$PROJECT_ROOT" inputs)
MAXNOUTPUTS=$(python3 "$IPLUG2_ROOT/Scripts/parse_iostr.py" "$PROJECT_ROOT" outputs)

IS_INSTRUMENT="false"
if [ "$MAXNINPUTS" -eq "0" ]; then
  IS_INSTRUMENT="true"
fi

HOST_RESIZE="false"
if grep -q "PLUG_HOST_RESIZE 1" "$PROJECT_ROOT/config.h" 2>/dev/null; then
  HOST_RESIZE="true"
fi

DOES_MIDI_IN="false"
if grep -q "PLUG_DOES_MIDI_IN 1" "$PROJECT_ROOT/config.h" 2>/dev/null; then
  DOES_MIDI_IN="true"
fi

DOES_MIDI_OUT="false"
if grep -q "PLUG_DOES_MIDI_OUT 1" "$PROJECT_ROOT/config.h" 2>/dev/null; then
  DOES_MIDI_OUT="true"
fi

PLUG_WIDTH=$(sed -nE 's/^#define[[:space:]]+PLUG_WIDTH[[:space:]]+([0-9]+).*/\1/p' "$PROJECT_ROOT/config.h" | head -n 1)
PLUG_HEIGHT=$(sed -nE 's/^#define[[:space:]]+PLUG_HEIGHT[[:space:]]+([0-9]+).*/\1/p' "$PROJECT_ROOT/config.h" | head -n 1)
if [ -z "$PLUG_WIDTH" ] || [ -z "$PLUG_HEIGHT" ]; then
  echo "WARNING: could not parse PLUG_WIDTH/PLUG_HEIGHT from $PROJECT_ROOT/config.h, defaulting to 600x600" >&2
fi
PLUG_WIDTH=${PLUG_WIDTH:-600}
PLUG_HEIGHT=${PLUG_HEIGHT:-600}

# Shared placeholder substitutions (NAME_PLACEHOLDER_LC must run before NAME_PLACEHOLDER).
COMMON_SUBS=(-e "s/NAME_PLACEHOLDER_LC/$PROJECT_NAME_LC/g"
             -e "s/NAME_PLACEHOLDER/$PROJECT_NAME/g"
             -e "s/MAXNINPUTS_PLACEHOLDER/$MAXNINPUTS/g"
             -e "s/MAXNOUTPUTS_PLACEHOLDER/$MAXNOUTPUTS/g"
             -e "s/IS_INSTRUMENT_PLACEHOLDER/$IS_INSTRUMENT/g"
             -e "s/HOST_RESIZE_PLACEHOLDER/$HOST_RESIZE/g"
             -e "s/DOES_MIDI_IN_PLACEHOLDER/$DOES_MIDI_IN/g"
             -e "s/DOES_MIDI_OUT_PLACEHOLDER/$DOES_MIDI_OUT/g")

# Copy and process bundle template.
cp "$IPLUG2_ROOT/IPlug/WEB/TemplateWasm/scripts/IPlugWasmBundle.js.template" "scripts/$PROJECT_NAME-bundle.js"
sed -i.bak "${COMMON_SUBS[@]}" -e "s/HAS_UI_PLACEHOLDER/true/g" "scripts/$PROJECT_NAME-bundle.js"

# Copy and process processor template.
cp "$IPLUG2_ROOT/IPlug/WEB/TemplateWasm/scripts/IPlugWasmProcessor.js.template" "scripts/$PROJECT_NAME-processor.js"
sed -i.bak -e "s/NAME_PLACEHOLDER_LC/$PROJECT_NAME_LC/g" -e "s/NAME_PLACEHOLDER/$PROJECT_NAME/g" "scripts/$PROJECT_NAME-processor.js"

# Copy shared host controls.
cp "$IPLUG2_ROOT/IPlug/WEB/TemplateWasm/scripts/IPlugWasmHostControls.js" scripts/IPlugWasmHostControls.js

# Copy and process WebView HTML template.
cp "$IPLUG2_ROOT/IPlug/WEB/TemplateWasm/webview.html" index.html
sed -i.bak "${COMMON_SUBS[@]}" \
  -e "s/PLUG_WIDTH_PLACEHOLDER/$PLUG_WIDTH/g" \
  -e "s/PLUG_HEIGHT_PLACEHOLDER/$PLUG_HEIGHT/g" index.html

# Copy and process CSS/dev server.
cp "$IPLUG2_ROOT/IPlug/WEB/TemplateWasm/styles/style.css" styles/style.css
sed -i.bak "s/NAME_PLACEHOLDER_LC/$PROJECT_NAME_LC/g" styles/style.css
cp "$IPLUG2_ROOT/IPlug/WEB/TemplateWasm/serve.py" serve.py

# Clean up sed backup files anywhere under the build dir.
find . -name '*.bak' -type f -delete

echo ""
echo "============================================================"
echo "BUILD COMPLETE"
echo "============================================================"
echo ""
echo "Output: build-web-wasm/"
echo ""
echo "Payload:"
find . -maxdepth 3 -mindepth 1 -name .git -type d \! -prune -o \! -name .DS_Store -type f -exec du -hs {} \;

echo ""
echo "============================================================"
echo "IMPORTANT: Server Requirements"
echo "============================================================"
echo "Your server MUST send these headers for SharedArrayBuffer:"
echo "  Cross-Origin-Opener-Policy: same-origin"
echo "  Cross-Origin-Embedder-Policy: require-corp"
echo ""
echo "Use the staged dev server or another server configured with those headers:"
echo "  python3 serve.py 8080"
echo "============================================================"

if [ "$LAUNCH_EMRUN" -eq 1 ]; then
  echo ""
  echo "Launching browser with emrun..."
  emrun --browser "$EMRUN_BROWSER" --no_emrun_detect index.html
else
  echo ""
  echo "Not launching browser (use 'on' argument to launch)"
fi
