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
cd build-web

if [ -f imgs.js ]
then
  rm imgs.js
fi

if [ -f imgs@2x.js ]
then
  rm imgs@2x.js
fi

if [ -f fonts.js ]
then
  rm fonts.js
fi

if [ -f presets.js ]
then
  rm presets.js
fi

python $EMSCRIPTEN/tools/file_packager.py fonts.data --preload ../resources/fonts/ --exclude .DS_store --js-output=fonts.js
echo "if(window.devicePixelRatio == 1) {\n" > imgs.js
python $EMSCRIPTEN/tools/file_packager.py imgs.data --use-preload-plugins --preload ../resources/img/ --use-preload-cache --indexedDB-name="/IGraphicsTest_pkg" --exclude *@2x.png .DS_store >> imgs.js
echo "\n}" >> imgs.js
# @ package @2x resources into separate .data file
mkdir ./2x/
cp ../resources/img/*@2x* ./2x
echo "if(window.devicePixelRatio > 1) {\n" > imgs@2x.js
#--use-preload-cache --indexedDB-name="/IGraphicsTest_data"
python $EMSCRIPTEN/tools/file_packager.py imgs@2x.data --use-preload-plugins --preload ./2x@/resources/img/ --use-preload-cache --indexedDB-name="/IGraphicsTest_pkg" --exclude .DS_store >> imgs@2x.js
echo "\n}" >> imgs@2x.js
rm -r ./2x

cd ..
echo -

# if [ "$websocket" -eq "0" ]
# then
#   echo MAKING  - WAM WASM MODULE -----------------------------
#   emmake make --makefile projects/IGraphicsTest-wam-processor.mk
#
#   if [ $? -ne "0" ]
#   then
#     echo IPlugWAM WASM compilation failed
#     exit 1
#   fi
#
#   cd build-web/scripts
#   # thanks to Steven Yi / Csound
#   echo "
#   fs = require('fs');
#   let wasmData = fs.readFileSync(\"IGraphicsTest-WAM.wasm\");
#   let wasmStr = wasmData.join(\",\");
#   let wasmOut = \"AudioWorkletGlobalScope.WAM = AudioWorkletGlobalScope.WAM || {}\\\n\";
#   wasmOut += \"AudioWorkletGlobalScope.WAM.IPlug = { ENVIRONMENT: 'WEB' }\\\n\";
#   wasmOut += \"AudioWorkletGlobalScope.WAM.IPlug.wasmBinary = new Uint8Array([\" + wasmStr + \"]);\";
#   fs.writeFileSync(\"IGraphicsTest-WAM.wasm.js\", wasmOut);
#   // later we will possibly switch to base64
#   // as suggested by Stephane Letz / Faust
#   // let base64 = wasmData.toString(\"base64\");
#   // fs.writeFileSync(\"IGraphicsTest-WAM.base64.js\", base64);
#   " > encode-wasm.js
#
#   node encode-wasm.js
#   rm encode-wasm.js
# #   rm IGraphicsTest-WAM.wasm
#
#   cp ../../../../Dependencies/IPlug/WAM_SDK/wamsdk/*.js .
#   cp ../../../../Dependencies/IPlug/WAM_AWP/*.js .
#   cp ../../../../IPlug/WEB/Template/scripts/IPlugWAM-awn.js IGraphicsTest-awn.js
#   sed -i.bak s/NAME_PLACEHOLDER/IGraphicsTest/g IGraphicsTest-awn.js
#   cp ../../../../IPlug/WEB/Template/scripts/IPlugWAM-awp.js IGraphicsTest-awp.js
#   sed -i.bak s/NAME_PLACEHOLDER/IGraphicsTest/g IGraphicsTest-awp.js
#   rm *.bak
#   cd ..
#
#   #copy in the template html - comment if you have customised the html
#   cp ../../../IPlug/WEB/Template/IPlugWAM-standalone.html index.html
#   sed -i.bak s/NAME_PLACEHOLDER/IGraphicsTest/g index.html
#   rm *.bak
#
#   cp ../../../IPlug/WEB/Template/favicon.ico favicon.ico
#
# else
  #copy in the template html for websocket - comment if you have customised the html
  cd build-web
  pwd
  cp ../../../IPlug/WEB/Template/IPlugWeb-remote.html index.html
  sed -i.bak s/NAME_PLACEHOLDER/IGraphicsTest/g index.html
  rm *.bak
# fi

cd ../

echo
echo MAKING  - WEB WASM MODULE -----------------------------

emmake make --makefile projects/IGraphicsTest-wam-controller.mk EXTRA_CFLAGS=-DWEBSOCKET_CLIENT=$websocket

if [ $? -ne "0" ]
then
  echo IPlugWEB WASM compilation failed
  exit 1
fi

# TODO: why is this a problem on mac?
# if [ "$(uname)" == "Darwin" ]
# then
#   mv build-web/scripts/*.wasm build-web
# fi

cd build-web

echo payload:
find . -maxdepth 2 -mindepth 1 -exec du -hs {} \;
du -hc

if [ "$websocket" -eq "1" ]
then
  emrun --browser chrome --no_server --port=8001 index.html
else
  emrun --browser chrome --no_emrun_detect index.html
# emrun --browser firefox index.html
fi
