#!/bin/sh

cd "$(dirname "$0")"

cd ..

if [ "$1" == "websocket" ]
then
  websocket=1
else
  websocket=0
fi

if [ -d build-web/.git ]
then
  # trash only the scripts folder
  if [ -d build-web/scripts ]
  then
    rm -r build-web/scripts
  fi
else
  # trash the whole build-web folder
  if [ -d build-web ]
  then
    rm -r build-web
  fi

  mkdir build-web
fi

mkdir build-web/scripts

echo BUNDLING RESOURCES -----------------------------
python $EMSCRIPTEN/tools/file_packager.py ./build-web/resources.data --use-preload-plugins --preload ./resources/img@/ --preload ./resources/fonts@/ --js-output=./build-web/resources.js
echo -

if [ "$websocket" -eq "0" ]
then
  echo MAKING  - WAM WASM MODULE -----------------------------
  emmake make --makefile projects/IPlugEffect-wam-processor.mk

  if [ $? -ne "0" ]
  then
    echo IPlugWAM WASM compilation failed
    exit 1
  fi

  cd build-web/scripts
  # thanks to Steven Yi / Csound
  echo "
  fs = require('fs');
  let wasmData = fs.readFileSync(\"IPlugEffect-WAM.wasm\");
  let wasmStr = wasmData.join(\",\");
  let wasmOut = \"AudioWorkletGlobalScope.WAM = AudioWorkletGlobalScope.WAM || {}\\\n\";
  wasmOut += \"AudioWorkletGlobalScope.WAM.IPlug = { ENVIRONMENT: 'WEB' }\\\n\";
  wasmOut += \"AudioWorkletGlobalScope.WAM.IPlug.wasmBinary = new Uint8Array([\" + wasmStr + \"]);\";
  fs.writeFileSync(\"IPlugEffect-WAM.wasm.js\", wasmOut);
  // later we will possibly switch to base64
  // as suggested by Stephane Letz / Faust
  // let base64 = wasmData.toString(\"base64\");
  // fs.writeFileSync(\"IPlugEffect-WAM.base64.js\", base64);
  " > encode-wasm.js

  node encode-wasm.js
  rm encode-wasm.js
  rm IPlugEffect-WAM.wasm

  cp ../../../../Dependencies/IPlug/WAM_SDK/wamsdk/*.js .
  cp ../../../../Dependencies/IPlug/WAM_AWP/*.js .
  cp ../../../../IPlug/WEB/Template/scripts/IPlugWAM-awn.js IPlugEffect-awn.js
  sed -i.bak s/IPlugWAM/IPlugEffect/g IPlugEffect-awn.js
  cp ../../../../IPlug/WEB/Template/scripts/IPlugWAM-awp.js IPlugEffect-awp.js
  sed -i.bak s/IPlugWAM/IPlugEffect/g IPlugEffect-awp.js
  rm *.bak
  cd ..

  #copy in the template html - comment if you have customised the html
  cp ../../../IPlug/WEB/Template/IPlugWAM-standalone.html index.html
  sed -i.bak s/IPlugWAM/IPlugEffect/g index.html
  rm *.bak

  cp ../../../IPlug/WEB/Template/favicon.ico favicon.ico

else
  #copy in the template html for websocket - comment if you have customised the html
  cd build-web
  pwd
  cp ../../../IPlug/WEB/Template/IPlugWeb-remote.html index.html
  sed -i.bak s/IPlugWEB/IPlugEffect/g index.html
  rm *.bak
fi

cd ../

echo
echo MAKING  - WEB WASM MODULE -----------------------------

emmake make --makefile projects/IPlugEffect-wam-controller.mk EXTRA_CFLAGS=-DWEBSOCKET_CLIENT=$websocket

if [ $? -ne "0" ]
then
  echo IPlugWEB WASM compilation failed
  exit 1
fi

mv build-web/scripts/*.wasm build-web

cd build-web

echo payload:
du -sch * --exclude=.git

if [ "$websocket" -eq "1" ]
then
  emrun --browser chrome --no_server --port=8001 index.html
else
  emrun --browser chrome --no_emrun_detect index.html
# emrun --browser firefox index.html
fi
