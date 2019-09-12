#!/bin/sh

git clone https://github.com/RazrFalcon/resvg.git ../Build/src/resvg

cd ../Build/src/resvg
cargo build --release --features "cairo-backend"
cd capi
mkdir .cargo
echo [build] > .cargo/config
ls -la $AGENT_BUILDDIRECTORY/s/Dependencies/Build/mac/lib/
echo rustflags = [\"-C\", \"link-args=-L$AGENT_BUILDDIRECTORY/s/Dependencies/Build/mac/lib/ -lpixman-1\"] >> .cargo/config
# brew install cairo harfbuzz
cargo build --verbose --release --features "cairo-backend"
cd ..
sed -i.bak s/"#include <cairo.h>"/"#include <cairo\/cairo.h>"/g capi/include/resvg.h
cp capi/include/resvg.h ../../mac/include/resvg.h 
mv target/release/libresvg.a ../../mac/lib