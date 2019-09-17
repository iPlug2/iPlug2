#!/bin/sh

cd ../Build/src/resvg

SKIA_DIR=/Users/oli/Dev/MyPlugins/Dependencies/Build/src/skia/ SKIA_LIB_DIR=/Users/oli/Dev/MyPlugins/Dependencies/Build/mac/lib cargo build --release --features "skia-backend cairo-backend"

cd capi

if [ -d .cargo ]; then
  rm -r .cargo
fi

echo ---------------------------------
mkdir .cargo
echo [build] > .cargo/config
echo rustflags = [\"-C\", \"link-arg=-L/Users/oli/Dev/MyPlugins/Dependencies/Build/mac/lib\", \"-C\", \"link-arg=-lpixman-1\"] >> .cargo/config

SKIA_DIR=/Users/oli/Dev/MyPlugins/Dependencies/Build/src/skia/ SKIA_LIB_DIR=/Users/oli/Dev/MyPlugins/Dependencies/Build/mac/lib cargo build --verbose --release --features "skia-backend cairo-backend"
cd ..
sed -i.bak s/"#include <cairo.h>"/"#include <cairo\/cairo.h>"/g capi/include/resvg.h
cp capi/include/resvg.h ../../mac/include/resvg.h 
mv target/release/libresvg.a ../../mac/lib