#!/bin/sh

pkill -f python

if [ -d build-web ]
then
  rm -r build-web
fi

mkdir build-web

emmake make

if [ -a build-web/IGraphicsTest.wasm ]
then
  cp -r resources/img build-web
  cd build-web
  emrun --browser chrome_canary --serve_after_close IGraphicsTest.html
fi
