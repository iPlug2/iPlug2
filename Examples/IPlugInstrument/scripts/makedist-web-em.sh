#!/bin/bash

# makedist-web-em.sh builds a Web version of an iPlug2 project using Emscripten's native AudioWorklet
# This is a simplified alternative to makedist-web.sh that doesn't require the WAM SDK
#
# IMPORTANT: The server must send these headers for SharedArrayBuffer support:
#   Cross-Origin-Opener-Policy: same-origin
#   Cross-Origin-Embedder-Policy: require-corp
#
# Arguments:
# 1st argument: "on" or "off" - whether to launch emrun after compilation
# 2nd argument: site origin (default: "/")
# 3rd argument: browser - "chrome", "safari", "firefox" (default: "chrome")

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
IPLUG2_ROOT=../../..
PROJECT_ROOT=$SCRIPT_DIR/..
IPLUG2_ROOT=$SCRIPT_DIR/$IPLUG2_ROOT

PROJECT_NAME=IPlugInstrument
EMRUN_BROWSER=chrome
LAUNCH_EMRUN=1
EMRUN_SERVER_PORT=8001
SITE_ORIGIN="/"
DEBUG_BUILD=0

cd $PROJECT_ROOT

# Parse arguments
for arg in "$@"; do
  case $arg in
    --debug)
      DEBUG_BUILD=1
      ;;
    off)
      LAUNCH_EMRUN=0
      ;;
  esac
done

if [ "$#" -ge 2 ]; then
  SITE_ORIGIN=${2}
fi

if [ "$#" -ge 3 ]; then
  EMRUN_BROWSER=${3}
fi

# Setup build directory
if [ -d build-web-em/.git ]; then
  if [ -d build-web-em/scripts ]; then rm -r build-web-em/scripts; fi
else
  if [ -d build-web-em ]; then rm -r build-web-em; fi
  mkdir build-web-em
fi

mkdir -p build-web-em/scripts
mkdir -p build-web-em/styles

echo BUNDLING RESOURCES -----------------------------

if [ -f ./build-web-em/imgs.js ]; then rm ./build-web-em/imgs.js; fi
if [ -f ./build-web-em/imgs@2x.js ]; then rm ./build-web-em/imgs@2x.js; fi
if [ -f ./build-web-em/svgs.js ]; then rm ./build-web-em/svgs.js; fi
if [ -f ./build-web-em/fonts.js ]; then rm ./build-web-em/fonts.js; fi

FILE_PACKAGER=$EMSDK/upstream/emscripten/tools/file_packager.py

# Package fonts
FOUND_FONTS=0
if [ -d ./resources/fonts ] && [ "$(ls -A ./resources/fonts/*.ttf 2>/dev/null)" ]; then
  FOUND_FONTS=1
  python3 $FILE_PACKAGER fonts.data --preload ./resources/fonts/ --exclude *DS_Store --js-output=./fonts.js
fi

# Package svgs
FOUND_SVGS=0
if [ -d ./resources/img ] && [ "$(ls -A ./resources/img/*.svg 2>/dev/null)" ]; then
  FOUND_SVGS=1
  python3 $FILE_PACKAGER svgs.data --preload ./resources/img/ --exclude *.png --exclude *DS_Store --js-output=./svgs.js
fi

# Package @1x pngs
FOUND_PNGS=0
if [ -d ./resources/img ] && [ "$(ls -A ./resources/img/*.png 2>/dev/null)" ]; then
  FOUND_PNGS=1
  python3 $FILE_PACKAGER imgs.data --use-preload-plugins --preload ./resources/img/ --use-preload-cache --indexedDB-name="/$PROJECT_NAME_pkg" --exclude *DS_Store --exclude *@2x.png --exclude *.svg >> ./imgs.js
fi

# Package @2x pngs into separate .data file
FOUND_2XPNGS=0
if [ -d ./resources/img ] && [ "$(ls -A ./resources/img/*@2x*.png 2>/dev/null)" ]; then
  FOUND_2XPNGS=1
  mkdir ./build-web-em/2x/
  cp ./resources/img/*@2x* ./build-web-em/2x
  python3 $FILE_PACKAGER imgs@2x.data --use-preload-plugins --preload ./2x@/resources/img/ --use-preload-cache --indexedDB-name="/$PROJECT_NAME_pkg" --exclude *DS_Store >> ./imgs@2x.js
  rm -r ./build-web-em/2x
fi

# Move packaged files
if [ -f ./imgs.js ]; then mv ./imgs.js ./build-web-em/imgs.js; fi
if [ -f ./imgs@2x.js ]; then mv ./imgs@2x.js ./build-web-em/imgs@2x.js; fi
if [ -f ./svgs.js ]; then mv ./svgs.js ./build-web-em/svgs.js; fi
if [ -f ./fonts.js ]; then mv ./fonts.js ./build-web-em/fonts.js; fi

if [ -f ./imgs.data ]; then mv ./imgs.data ./build-web-em/imgs.data; fi
if [ -f ./imgs@2x.data ]; then mv ./imgs@2x.data ./build-web-em/imgs@2x.data; fi
if [ -f ./svgs.data ]; then mv ./svgs.data ./build-web-em/svgs.data; fi
if [ -f ./fonts.data ]; then mv ./fonts.data ./build-web-em/fonts.data; fi

if [ "$DEBUG_BUILD" == "1" ]; then
  echo MAKING EMSCRIPTEN AUDIOWORKLET WASM MODULE \(DEBUG\) -----------------------------
else
  echo MAKING EMSCRIPTEN AUDIOWORKLET WASM MODULE -----------------------------
fi

cd $PROJECT_ROOT/projects
if [ "$DEBUG_BUILD" == "1" ]; then
  emmake make --makefile $PROJECT_NAME-em.mk DEBUG=1
else
  emmake make --makefile $PROJECT_NAME-em.mk
fi

if [ $? -ne "0" ]; then
  echo "Emscripten AudioWorklet WASM compilation failed"
  exit 1
fi

cd $PROJECT_ROOT/build-web-em

# Copy the template HTML
cp $IPLUG2_ROOT/IPlug/WEB/TemplateEm/index.html index.html
PROJECT_NAME_LC=$(echo "$PROJECT_NAME" | tr '[:upper:]' '[:lower:]')
sed -i.bak s/NAME_PLACEHOLDER_LC/$PROJECT_NAME_LC/g index.html
sed -i.bak s/NAME_PLACEHOLDER/$PROJECT_NAME/g index.html

# Update I/O details based on config.h channel io str
MAXNINPUTS=$(python3 $IPLUG2_ROOT/Scripts/parse_iostr.py "$PROJECT_ROOT" inputs)
MAXNOUTPUTS=$(python3 $IPLUG2_ROOT/Scripts/parse_iostr.py "$PROJECT_ROOT" outputs)

if [ "$MAXNINPUTS" = "" ] || [ "$MAXNINPUTS" = "0" ]; then
  MAXNINPUTS="0"
fi

sed -i.bak s/"MAXNINPUTS_PLACEHOLDER"/"$MAXNINPUTS"/g index.html
sed -i.bak s/"MAXNOUTPUTS_PLACEHOLDER"/"$MAXNOUTPUTS"/g index.html

# Check if plugin supports host resize
HOST_RESIZE=$(grep -E "^\s*#define\s+PLUG_HOST_RESIZE\s+" "$PROJECT_ROOT/config.h" | awk '{print $3}')
if [ "$HOST_RESIZE" = "1" ]; then
  sed -i.bak s/HOST_RESIZE_CLASS_PLACEHOLDER/resizable/g index.html
else
  sed -i.bak s/HOST_RESIZE_CLASS_PLACEHOLDER//g index.html
fi

# Comment out resource scripts that don't exist
if [ $FOUND_FONTS -eq "0" ]; then sed -i.bak s/'<script async src="fonts.js"><\/script>'/'<!--<script async src="fonts.js"><\/script>-->'/g index.html; fi
if [ $FOUND_SVGS -eq "0" ]; then sed -i.bak s/'<script async src="svgs.js"><\/script>'/'<!--<script async src="svgs.js"><\/script>-->'/g index.html; fi
if [ $FOUND_PNGS -eq "0" ]; then sed -i.bak s/'<script async src="imgs.js"><\/script>'/'<!--<script async src="imgs.js"><\/script>-->'/g index.html; fi
if [ $FOUND_2XPNGS -eq "0" ]; then sed -i.bak s/'<script async src="imgs@2x.js"><\/script>'/'<!--<script async src="imgs@2x.js"><\/script>-->'/g index.html; fi

rm -f *.bak

# Copy styles and controller script
cp $IPLUG2_ROOT/IPlug/WEB/TemplateEm/styles/style.css styles/style.css
cp $IPLUG2_ROOT/IPlug/WEB/TemplateEm/favicon.ico favicon.ico
cp $IPLUG2_ROOT/IPlug/WEB/TemplateEm/scripts/IPlugController.js scripts/IPlugController.js

# Print payload
echo "Build complete! Payload:"
find . -maxdepth 2 -mindepth 1 -name .git -type d \! -prune -o \! -name .DS_Store -type f -exec du -hs {} \;

echo ""
echo "IMPORTANT: Server must send these headers for SharedArrayBuffer support:"
echo "  Cross-Origin-Opener-Policy: same-origin"
echo "  Cross-Origin-Embedder-Policy: require-corp"
echo ""

# Launch emrun if requested
if [ "$LAUNCH_EMRUN" -eq "1" ]; then
  # emrun with COOP/COEP headers
  emrun --browser $EMRUN_BROWSER --no_emrun_detect \
    --serve_after_close \
    --serve_root . \
    index.html
else
  echo "Not running emrun. To test locally with proper headers, use:"
  echo "  npx serve -p 8080 --cors -n"
  echo "  Or configure your server to send COOP/COEP headers"
fi
