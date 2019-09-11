#!/bin/sh

git clone https://github.com/RazrFalcon/resvg.git ../Build/src/resvg

cd ../Build/src/resvg
cargo build --release --features "cairo-backend"
cd capi
mkdir .cargo
echo [build] > .cargo/config
echo rustflags = [\"-C\", \"link-args=-L../../../mac/lib/\"] >> .cargo/config
cargo build --release --features "cairo-backend"
cd ..
mv capi/include/resvg.h ../../mac/include
mv target/release/libresvg.a ../../mac/lib