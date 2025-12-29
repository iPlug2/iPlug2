#!/bin/bash

# makedist-web-em.sh builds a headless Web version of IPlugConvoEngine
# using Emscripten's native AudioWorklet (no IGraphics)
#
# IMPORTANT: The server must send these headers for SharedArrayBuffer support:
#   Cross-Origin-Opener-Policy: same-origin
#   Cross-Origin-Embedder-Policy: require-corp
#
# Arguments:
# 1st argument: "on" or "off" - whether to launch emrun after compilation
# 2nd argument: browser - "chrome", "safari", "firefox" (default: "chrome")

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
IPLUG2_ROOT=../../..
PROJECT_ROOT=$SCRIPT_DIR/..
IPLUG2_ROOT=$SCRIPT_DIR/$IPLUG2_ROOT

PROJECT_NAME=IPlugConvoEngine
EMRUN_BROWSER=chrome
LAUNCH_EMRUN=1
EMRUN_SERVER_PORT=8001

cd $PROJECT_ROOT

if [ "$1" = "off" ]; then
  LAUNCH_EMRUN=0
fi

if [ "$#" -ge 2 ]; then
  EMRUN_BROWSER=${2}
fi

# Setup build directory
if [ -d build-web-em/.git ]; then
  if [ -d build-web-em/scripts ]; then rm -r build-web-em/scripts; fi
else
  if [ -d build-web-em ]; then rm -r build-web-em; fi
  mkdir build-web-em
fi

mkdir -p build-web-em/scripts

echo MAKING EMSCRIPTEN AUDIOWORKLET WASM MODULE \(HEADLESS\) -----------------------------

cd $PROJECT_ROOT/projects
emmake make --makefile $PROJECT_NAME-em.mk

if [ $? -ne "0" ]; then
  echo "Emscripten AudioWorklet WASM compilation failed"
  exit 1
fi

cd $PROJECT_ROOT/build-web-em

# Copy the headless template HTML
cp $IPLUG2_ROOT/IPlug/WEB/TemplateEmHeadless/index.html index.html
sed -i.bak s/PLUG_NAME/$PROJECT_NAME/g index.html
rm -f *.bak

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
  emrun --browser $EMRUN_BROWSER --no_emrun_detect \
    --serve_after_close \
    --serve_root . \
    index.html
else
  echo "Not running emrun. To test locally with proper headers, use:"
  echo "  npx serve -p 8080 --cors -n"
  echo "  Or configure your server to send COOP/COEP headers"
fi
