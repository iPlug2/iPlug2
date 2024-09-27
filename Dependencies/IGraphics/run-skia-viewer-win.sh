#!/bin/sh

# This script is for building and running the Skia Viewer app

DEPOT_TOOLS_PATH=../Build/tmp/depot_tools

if [ ! -d $DEPOT_TOOLS_PATH ]; then
  git clone 'https://chromium.googlesource.com/chromium/tools/depot_tools.git' $DEPOT_TOOLS_PATH
fi

export PATH="${PWD}/$DEPOT_TOOLS_PATH:${PATH}"

cd ../Build/src/skia
bin/gn gen ../../tmp/skia/viewer --args='
is_debug = false
skia_enable_gpu = true
skia_use_dawn = true
skia_use_direct3d = true
skia_use_gl = true
skia_enable_graphite = true
'
ninja -v -C ../../tmp/skia/viewer viewer
../../tmp/skia/viewer/viewer --resourcePath resources
