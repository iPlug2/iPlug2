#!/bin/sh

git clone https://github.com/RazrFalcon/resvg.git ../Build/src/resvg

cd ../Build/src/resvg
cargo build --release --features "cairo-backend"