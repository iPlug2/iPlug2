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
python $EMSCRIPTEN/tools/file_packager.py svgs.data --preload ../resources/img/ --exclude *.png .DS_store --js-output=svgs.js

echo "if(window.devicePixelRatio == 1) {\n" > imgs.js
python $EMSCRIPTEN/tools/file_packager.py imgs.data --use-preload-plugins --preload ../resources/img/ --use-preload-cache --indexedDB-name="/IPlugFaustDSP_pkg" --exclude *@2x.png .DS_store *.svg >> imgs.js
echo "\n}" >> imgs.js
# @ package @2x resources into separate .data file
mkdir ./2x/
cp ../resources/img/*@2x* ./2x
echo "if(window.devicePixelRatio > 1) {\n" > imgs@2x.js
#--use-preload-cache --indexedDB-name="/IPlugFaustDSP_data"
python $EMSCRIPTEN/tools/file_packager.py imgs@2x.data --use-preload-plugins --preload ./2x@/resources/img/ --use-preload-cache --indexedDB-name="/IPlugFaustDSP_pkg" --exclude .DS_store >> imgs@2x.js
echo "\n}" >> imgs@2x.js
rm -r ./2x

cd ..
echo -

if [ "$websocket" -eq "0" ]
then
  echo MAKING  - WAM WASM MODULE -----------------------------
  emmake make --makefile projects/IPlugFaustDSP-wam-processor.mk

  if [ $? -ne "0" ]
  then
    echo IPlugWAM WASM compilation failed
    exit 1
  fi

  cd build-web/scripts

  echo "AudioWorkletGlobalScope.WAM = AudioWorkletGlobalScope.WAM || {}; AudioWorkletGlobalScope.WAM.IPlugFaustDSP = { ENVIRONMENT: 'WEB' };" > IPlugFaustDSP-wam.tmp.js;
  cat IPlugFaustDSP-wam.js >> IPlugFaustDSP-wam.tmp.js
  mv IPlugFaustDSP-wam.tmp.js IPlugFaustDSP-wam.js

  cp ../../../../Dependencies/IPlug/WAM_SDK/wamsdk/*.js .
  cp ../../../../Dependencies/IPlug/WAM_AWP/*.js .
  cp ../../../../IPlug/WEB/Template/scripts/IPlugWAM-awn.js IPlugFaustDSP-awn.js
  sed -i.bak s/NAME_PLACEHOLDER/IPlugFaustDSP/g IPlugFaustDSP-awn.js
  cp ../../../../IPlug/WEB/Template/scripts/IPlugWAM-awp.js IPlugFaustDSP-awp.js
  sed -i.bak s/NAME_PLACEHOLDER/IPlugFaustDSP/g IPlugFaustDSP-awp.js
  rm *.bak
  cd ..

  #copy in the template html - comment if you have customised the html
  cp ../../../IPlug/WEB/Template/IPlugWAM-standalone.html index.html
  sed -i.bak s/NAME_PLACEHOLDER/IPlugFaustDSP/g index.html
  rm *.bak

  cp ../../../IPlug/WEB/Template/favicon.ico favicon.ico

else
  #copy in the template html for websocket - comment if you have customised the html
  cd build-web
  pwd
  cp ../../../IPlug/WEB/Template/IPlugWeb-remote.html index.html
  sed -i.bak s/IPlugWEB/IPlugFaustDSP/g index.html
  rm *.bak
fi

cd ../

echo
echo MAKING  - WEB WASM MODULE -----------------------------

emmake make --makefile projects/IPlugFaustDSP-wam-controller.mk EXTRA_CFLAGS=-DWEBSOCKET_CLIENT=$websocket

if [ $? -ne "0" ]
then
  echo IPlugWEB WASM compilation failed
  exit 1
fi

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
