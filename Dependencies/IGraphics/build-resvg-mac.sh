#!/bin/sh

cd ../Build/src/resvg
export SKIA_DIR=$(PWD)/../skia
export SKIA_LIB_DIR=$(PWD)/../../mac/lib
export MACOSX_DEPLOYMENT_TARGET=10.9
cargo build --release --features "skia-backend cairo-backend"

cd capi

if [ -d .cargo ]; then
  rm -r .cargo
fi

echo ---------------------------------
mkdir .cargo
echo [build] > .cargo/config
echo rustflags = [\"-L$SKIA_LIB_DIR\", \"-ldylib=pixman-1\", \"-lframework=Metal\", \"-lframework=Foundation\"] >> .cargo/config

cargo build --verbose --release --features "skia-backend cairo-backend"
cd ..

mv target/release/libresvg.a ../../mac/lib