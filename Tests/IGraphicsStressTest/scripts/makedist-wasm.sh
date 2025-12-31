#!/bin/bash

# makedist-wasm.sh builds a Web version of an iPlug2 project using the wasm approach
# For headless plugins (no IGraphics), only the DSP module is built and the template
# auto-generates parameter controls.
#
# Arguments:
# 1st argument : either "on" or "off" - whether to launch emrun after compilation
# 2nd argument : browser - "chrome", "safari", or "firefox"

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
IPLUG2_ROOT=../../..
PROJECT_ROOT=$SCRIPT_DIR/..
IPLUG2_ROOT=$SCRIPT_DIR/$IPLUG2_ROOT

PROJECT_NAME=IGraphicsStressTest
PROJECT_NAME_LC=$(echo "$PROJECT_NAME" | tr '[:upper:]' '[:lower:]')
EMRUN_BROWSER=chrome
LAUNCH_EMRUN=1

# Check if plugin has UI (PLUG_HAS_UI in config.h)
HAS_UI=0
if grep -q "PLUG_HAS_UI 1" "$PROJECT_ROOT/config.h" 2>/dev/null; then
  HAS_UI=1
fi

cd $PROJECT_ROOT

if [ "$1" = "off" ]; then
  LAUNCH_EMRUN=0
fi

if [ "$#" -ge 2 ]; then
  EMRUN_BROWSER=${2}
fi

# Clean/create build directory
if [ -d build-web-wasm/.git ]; then
  # If there's a git repo, only trash the scripts folder
  if [ -d build-web-wasm/scripts ]; then rm -r build-web-wasm/scripts; fi
else
  if [ -d build-web-wasm ]; then rm -r build-web-wasm; fi
  mkdir build-web-wasm
fi

mkdir -p build-web-wasm/scripts
mkdir -p build-web-wasm/styles

echo "============================================================"
echo "BUNDLING RESOURCES"
echo "============================================================"

# Clean up old resource files
rm -f ./build-web-wasm/scripts/imgs.js
rm -f ./build-web-wasm/scripts/imgs@2x.js
rm -f ./build-web-wasm/scripts/svgs.js
rm -f ./build-web-wasm/scripts/fonts.js

FILE_PACKAGER=$EMSDK/upstream/emscripten/tools/file_packager.py

# Package fonts
FOUND_FONTS=0
if [ -d ./resources/fonts ] && [ "$(ls -A ./resources/fonts/*.ttf 2>/dev/null)" ]; then
  FOUND_FONTS=1
  echo "Packaging fonts..."
  python3 $FILE_PACKAGER fonts.data --preload ./resources/fonts/ --exclude *DS_Store --js-output=./fonts.js
  mv ./fonts.js ./build-web-wasm/scripts/fonts.js
  mv ./fonts.data ./build-web-wasm/fonts.data
fi

# Package SVGs
FOUND_SVGS=0
if [ -d ./resources/img ] && [ "$(ls -A ./resources/img/*.svg 2>/dev/null)" ]; then
  FOUND_SVGS=1
  echo "Packaging SVGs..."
  python3 $FILE_PACKAGER svgs.data --preload ./resources/img/ --exclude *.png --exclude *DS_Store --js-output=./svgs.js
  mv ./svgs.js ./build-web-wasm/scripts/svgs.js
  mv ./svgs.data ./build-web-wasm/svgs.data
fi

# Package @1x PNGs
FOUND_PNGS=0
if [ -d ./resources/img ] && [ "$(ls -A ./resources/img/*.png 2>/dev/null | grep -v @2x)" ]; then
  FOUND_PNGS=1
  echo "Packaging PNGs..."
  python3 $FILE_PACKAGER imgs.data --use-preload-plugins --preload ./resources/img/ --use-preload-cache --indexedDB-name="/${PROJECT_NAME}_pkg" --exclude *DS_Store --exclude *@2x.png --exclude *.svg --js-output=./imgs.js
  mv ./imgs.js ./build-web-wasm/scripts/imgs.js
  mv ./imgs.data ./build-web-wasm/imgs.data
fi

# Package @2x PNGs
FOUND_2XPNGS=0
if [ -d ./resources/img ] && [ "$(ls -A ./resources/img/*@2x*.png 2>/dev/null)" ]; then
  FOUND_2XPNGS=1
  echo "Packaging @2x PNGs..."
  mkdir -p ./build-web-wasm/2x/
  cp ./resources/img/*@2x* ./build-web-wasm/2x/
  cd build-web-wasm
  python3 $FILE_PACKAGER imgs@2x.data --use-preload-plugins --preload ./2x@/resources/img/ --use-preload-cache --indexedDB-name="/${PROJECT_NAME}_pkg" --exclude *DS_Store --js-output=./scripts/imgs@2x.js
  rm -r ./2x
  cd ..
fi

echo ""
echo "============================================================"
echo "BUILDING DSP WASM MODULE (AudioWorklet)"
echo "============================================================"

cd $PROJECT_ROOT/projects
emmake make --makefile $PROJECT_NAME-wasm-dsp.mk

if [ $? -ne 0 ]; then
  echo "ERROR: DSP WASM compilation failed"
  exit 1
fi

# Wrap DSP module for AudioWorklet scope
# AudioWorklet doesn't have 'self', so we shim it to 'globalThis'
# and expose the Module as globalThis.Module
cd $PROJECT_ROOT/build-web-wasm/scripts

echo "// AudioWorklet scope wrapper for iPlug2 Wasm DSP
// Shim 'self' and 'location' for AudioWorklet environment
var self = globalThis;
self.location = self.location || { href: 'https://localhost/' };
var Module = globalThis.Module = globalThis.Module || {};
" > $PROJECT_NAME-dsp.tmp.js

cat $PROJECT_NAME-dsp.js >> $PROJECT_NAME-dsp.tmp.js
mv $PROJECT_NAME-dsp.tmp.js $PROJECT_NAME-dsp.js

cd $PROJECT_ROOT/projects

# Only build UI module if plugin has IGraphics
if [ "$HAS_UI" -eq 1 ]; then
  echo ""
  echo "============================================================"
  echo "BUILDING UI WASM MODULE (IGraphics)"
  echo "============================================================"

  emmake make --makefile $PROJECT_NAME-wasm-ui.mk

  if [ $? -ne 0 ]; then
    echo "ERROR: UI WASM compilation failed"
    exit 1
  fi
else
  echo ""
  echo "============================================================"
  echo "HEADLESS PLUGIN - NO UI MODULE"
  echo "============================================================"
  echo "Plugin has PLUG_HAS_UI=0, skipping UI WASM build."
  echo "Parameter controls will be auto-generated in browser."
fi

cd $PROJECT_ROOT/build-web-wasm

echo ""
echo "============================================================"
echo "GENERATING JAVASCRIPT BUNDLE"
echo "============================================================"

# Get I/O configuration from config.h
MAXNINPUTS=$(python3 $IPLUG2_ROOT/Scripts/parse_iostr.py "$PROJECT_ROOT" inputs)
MAXNOUTPUTS=$(python3 $IPLUG2_ROOT/Scripts/parse_iostr.py "$PROJECT_ROOT" outputs)

# Determine if plugin is instrument (no audio inputs)
IS_INSTRUMENT="false"
if [ "$MAXNINPUTS" -eq "0" ]; then
  IS_INSTRUMENT="true"
fi

# Check if host resize is enabled
HOST_RESIZE="false"
if grep -q "PLUG_HOST_RESIZE 1" "$PROJECT_ROOT/config.h" 2>/dev/null; then
  HOST_RESIZE="true"
fi

# Check MIDI capabilities
DOES_MIDI_IN="false"
if grep -q "PLUG_DOES_MIDI_IN 1" "$PROJECT_ROOT/config.h" 2>/dev/null; then
  DOES_MIDI_IN="true"
fi

DOES_MIDI_OUT="false"
if grep -q "PLUG_DOES_MIDI_OUT 1" "$PROJECT_ROOT/config.h" 2>/dev/null; then
  DOES_MIDI_OUT="true"
fi

# Copy and process bundle template
# IMPORTANT: Replace NAME_PLACEHOLDER_LC first (longer match) before NAME_PLACEHOLDER
cp $IPLUG2_ROOT/IPlug/WEB/TemplateWasm/scripts/IPlugWasmBundle.js.template scripts/$PROJECT_NAME-bundle.js
sed -i.bak "s/NAME_PLACEHOLDER_LC/$PROJECT_NAME_LC/g" scripts/$PROJECT_NAME-bundle.js
sed -i.bak "s/NAME_PLACEHOLDER/$PROJECT_NAME/g" scripts/$PROJECT_NAME-bundle.js
sed -i.bak "s/MAXNINPUTS_PLACEHOLDER/$MAXNINPUTS/g" scripts/$PROJECT_NAME-bundle.js
sed -i.bak "s/MAXNOUTPUTS_PLACEHOLDER/$MAXNOUTPUTS/g" scripts/$PROJECT_NAME-bundle.js
sed -i.bak "s/IS_INSTRUMENT_PLACEHOLDER/$IS_INSTRUMENT/g" scripts/$PROJECT_NAME-bundle.js
sed -i.bak "s/HOST_RESIZE_PLACEHOLDER/$HOST_RESIZE/g" scripts/$PROJECT_NAME-bundle.js

# Set HAS_UI placeholder based on PLUG_HAS_UI
if [ "$HAS_UI" -eq 1 ]; then
  sed -i.bak "s/HAS_UI_PLACEHOLDER/true/g" scripts/$PROJECT_NAME-bundle.js
else
  sed -i.bak "s/HAS_UI_PLACEHOLDER/false/g" scripts/$PROJECT_NAME-bundle.js
fi

sed -i.bak "s/DOES_MIDI_IN_PLACEHOLDER/$DOES_MIDI_IN/g" scripts/$PROJECT_NAME-bundle.js
sed -i.bak "s/DOES_MIDI_OUT_PLACEHOLDER/$DOES_MIDI_OUT/g" scripts/$PROJECT_NAME-bundle.js

# Copy and process processor template
# IMPORTANT: Replace NAME_PLACEHOLDER_LC first (longer match) before NAME_PLACEHOLDER
cp $IPLUG2_ROOT/IPlug/WEB/TemplateWasm/scripts/IPlugWasmProcessor.js.template scripts/$PROJECT_NAME-processor.js
sed -i.bak "s/NAME_PLACEHOLDER_LC/$PROJECT_NAME_LC/g" scripts/$PROJECT_NAME-processor.js
sed -i.bak "s/NAME_PLACEHOLDER/$PROJECT_NAME/g" scripts/$PROJECT_NAME-processor.js

# Copy and process HTML template
# IMPORTANT: Replace NAME_PLACEHOLDER_LC first (longer match) before NAME_PLACEHOLDER
cp $IPLUG2_ROOT/IPlug/WEB/TemplateWasm/index.html index.html
sed -i.bak "s/NAME_PLACEHOLDER_LC/$PROJECT_NAME_LC/g" index.html
sed -i.bak "s/NAME_PLACEHOLDER/$PROJECT_NAME/g" index.html

# Comment out resource scripts that don't exist
if [ $FOUND_FONTS -eq 0 ]; then
  sed -i.bak 's/<!--FONTS--><script/<\!--<script/g' index.html
  sed -i.bak 's/fonts.js"><\/script>/fonts.js"><\/script>-->/g' index.html
fi
if [ $FOUND_SVGS -eq 0 ]; then
  sed -i.bak 's/<!--SVGS--><script/<\!--<script/g' index.html
  sed -i.bak 's/svgs.js"><\/script>/svgs.js"><\/script>-->/g' index.html
fi
if [ $FOUND_PNGS -eq 0 ]; then
  sed -i.bak 's/<!--IMGS--><script/<\!--<script/g' index.html
  sed -i.bak 's/imgs.js"><\/script>/imgs.js"><\/script>-->/g' index.html
fi
if [ $FOUND_2XPNGS -eq 0 ]; then
  sed -i.bak 's/<!--IMGS2X--><script/<\!--<script/g' index.html
  sed -i.bak 's/imgs@2x.js"><\/script>/imgs@2x.js"><\/script>-->/g' index.html
fi

# Copy and process CSS
cp $IPLUG2_ROOT/IPlug/WEB/TemplateWasm/styles/style.css styles/style.css
sed -i.bak "s/NAME_PLACEHOLDER_LC/$PROJECT_NAME_LC/g" styles/style.css

# Clean up backup files
rm -f *.bak scripts/*.bak styles/*.bak

echo ""
echo "============================================================"
echo "BUILD COMPLETE"
echo "============================================================"
echo ""
echo "Output: build-web-wasm/"
echo ""
echo "Payload:"
find . -maxdepth 2 -mindepth 1 -name .git -type d \! -prune -o \! -name .DS_Store -type f -exec du -hs {} \;

echo ""
echo "============================================================"
echo "IMPORTANT: Server Requirements"
echo "============================================================"
echo "Your server MUST send these headers for SharedArrayBuffer:"
echo "  Cross-Origin-Opener-Policy: same-origin"
echo "  Cross-Origin-Embedder-Policy: require-corp"
echo ""
echo "Use emrun or:"
echo "  npx serve -p 8080 --cors -n"
echo "  (with headers configured)"
echo "============================================================"

# Launch emrun
if [ "$LAUNCH_EMRUN" -eq 1 ]; then
  echo ""
  echo "Launching browser with emrun..."
  emrun --browser $EMRUN_BROWSER --no_emrun_detect index.html
else
  echo ""
  echo "Not launching browser (use 'on' argument to launch)"
fi
