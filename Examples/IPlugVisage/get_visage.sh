#!/bin/sh

set -e

git clone --recurse-submodules git@github.com:VitalAudio/visage.git
cd visage/
mkdir build
cd build
cmake -DVISAGE_BUILD_EXAMPLES=ON -DVISAGE_BUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
cd ../..