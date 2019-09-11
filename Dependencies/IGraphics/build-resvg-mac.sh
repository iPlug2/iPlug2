#!/bin/sh

git clone https://github.com/RazrFalcon/resvg.git ../Build/src/resvg

cd ../Build/src/resvg
cargo build --release --features "cairo-backend"
cd capi
mkdir .cargo
echo [build] > .cargo/config
echo rustflags = [\"-C\", \"link-args=-L../../../mac/lib/\"] >> .cargo/config
brew install cairo harfbuzz
cargo build --verbose --release --features "cairo-backend"
cd ..
sed -i.bak s/"#include <cairo.h>"/"#include <cairo/cairo.h>"/g capi/include/resvg.h
mv capi/include/resvg.h ../../mac/include
mv target/release/libresvg.a ../../mac/lib