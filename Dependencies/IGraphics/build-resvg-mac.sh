#!/bin/sh

cd ../Build/src/resvg
cargo build --release --features "cairo-backend"
cd capi
mkdir .cargo
echo [build] > .cargo/config
echo rustflags = [\"-C\", \"link-args=-L../../mac/lib/ -lpixman-1\"] >> .cargo/config

cargo build --verbose --release --features "cairo-backend"
cd ..
sed -i.bak s/"#include <cairo.h>"/"#include <cairo\/cairo.h>"/g capi/include/resvg.h
cp capi/include/resvg.h ../../mac/include/resvg.h 
mv target/release/libresvg.a ../../mac/lib