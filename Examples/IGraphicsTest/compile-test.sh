#!/bin/sh

pkill -f python

if [ -d build-web ]
then
  rm -r build-web
fi

mkdir build-web

emmake make

cp -r resources/img build-web
cd build-web

osascript <<EOF
tell application "Google Chrome Canary"
  open location "http://localhost:8000/IGraphicsTest.html"
  activate
end tell
EOF

python -m SimpleHTTPServer 8000
