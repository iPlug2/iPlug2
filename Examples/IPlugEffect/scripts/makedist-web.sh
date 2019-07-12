#!/bin/sh

PROJECT_NAME=IPlugEffect
WEBSOCKET_MODE=0
EMRUN_BROWSER=chrome
LAUNCH_EMRUN=1
EMRUN_SERVER=1
EMRUN_SERVER_PORT=8001
SITE_ORIGIN="/"

cd "$(dirname "$0")"

cd ..

if [ "$1" = "websocket" ]; then
  RUN_SERVER=0
  WEBSOCKET_MODE=1
elif [ "$1" = "off" ]; then
  LAUNCH_EMRUN=0
fi

if [ "$#" -eq 2 ]; then
  SITE_ORIGIN=${2}
fi

# check to see if the build web folder has its own git repo
if [ -d build-web/.git ]
then
  # if so trash only the scripts folder
  if [ -d build-web/scripts ]; then rm -r build-web/scripts; fi
else
  # otherwise trash the whole build-web folder
  if [ -d build-web ]; then rm -r build-web; fi

  mkdir build-web
fi

mkdir build-web/scripts

echo BUNDLING RESOURCES -----------------------------
cd build-web

if [ -f imgs.js ]; then rm imgs.js; fi
if [ -f imgs@2x.js ]; then rm imgs@2x.js; fi
if [ -f svgs.js ]; then rm svgs.js; fi
if [ -f fonts.js ]; then rm fonts.js; fi

#package fonts
FOUND_FONTS=0
if [ -f ../resources/fonts/*.ttf ]; then
  FOUND_FONTS=1
  python $EMSCRIPTEN/tools/file_packager.py fonts.data --preload ../resources/fonts/ --exclude *DS_Store --js-output=fonts.js
fi

#package svgs
FOUND_SVGS=0
if [ -f ../resources/img/*.svg ]; then
  FOUND_SVGS=1
  python $EMSCRIPTEN/tools/file_packager.py svgs.data --preload ../resources/img/ --exclude *.png --exclude *DS_Store --js-output=svgs.js
fi

#package @1x pngs
FOUND_PNGS=0
if [ -f ../resources/img/*.png ]; then
  FOUND_PNGS=1
  python $EMSCRIPTEN/tools/file_packager.py imgs.data --use-preload-plugins --preload ../resources/img/ --use-preload-cache --indexedDB-name="/$PROJECT_NAME_pkg" --exclude *DS_Store --exclude  *@2x.png --exclude  *.svg >> imgs.js
fi

# package @2x pngs into separate .data file
FOUND_2XPNGS=0
if [ -f ../resources/img/*@2x* ]; then
  FOUND_2XPNGS=1
  mkdir ./2x/
  cp ../resources/img/*@2x* ./2x
  python $EMSCRIPTEN/tools/file_packager.py imgs@2x.data --use-preload-plugins --preload ./2x@/resources/img/ --use-preload-cache --indexedDB-name="/$PROJECT_NAME_pkg" --exclude *DS_Store >> imgs@2x.js
  rm -r ./2x
fi

cd ..
echo -

echo MAKING  - WAM WASM MODULE -----------------------------
emmake make --makefile projects/$PROJECT_NAME-wam-processor.mk

if [ $? -ne "0" ]; then
  echo IPlugWAM WASM compilation failed
  exit 1
fi

cd build-web/scripts

# prefix the -wam.js script with scope 
echo "AudioWorkletGlobalScope.WAM = AudioWorkletGlobalScope.WAM || {}; AudioWorkletGlobalScope.WAM.$PROJECT_NAME = { ENVIRONMENT: 'WEB' };" > $PROJECT_NAME-wam.tmp.js;
cat $PROJECT_NAME-wam.js >> $PROJECT_NAME-wam.tmp.js
mv $PROJECT_NAME-wam.tmp.js $PROJECT_NAME-wam.js

# copy in WAM SDK and AudioWorklet polyfill scripts
cp ../../../../Dependencies/IPlug/WAM_SDK/wamsdk/*.js .
cp ../../../../Dependencies/IPlug/WAM_AWP/*.js .

# copy in template scripts
cp ../../../../IPlug/WEB/Template/scripts/IPlugWAM-awn.js $PROJECT_NAME-awn.js
cp ../../../../IPlug/WEB/Template/scripts/IPlugWAM-awp.js $PROJECT_NAME-awp.js

# replace NAME_PLACEHOLDER in the template -awn.js and -awp.js scripts
sed -i.bak s/NAME_PLACEHOLDER/$PROJECT_NAME/g $PROJECT_NAME-awn.js
sed -i.bak s/NAME_PLACEHOLDER/$PROJECT_NAME/g $PROJECT_NAME-awp.js

# replace ORIGIN_PLACEHOLDER in the template -awn.js script
sed -i.bak s,ORIGIN_PLACEHOLDER,$SITE_ORIGIN,g $PROJECT_NAME-awn.js
rm *.bak

cd ..

# copy in the template HTML - comment this out if you have customised the HTML
cp ../../../IPlug/WEB/Template/index.html index.html
sed -i.bak s/NAME_PLACEHOLDER/$PROJECT_NAME/g index.html

if [ $FOUND_FONTS -eq "0" ]; then sed -i.bak s/'<script async src="fonts.js"><\/script>'/'<!--<script async src="fonts.js"><\/script>-->'/g index.html; fi
if [ $FOUND_SVGS -eq "0" ]; then sed -i.bak s/'<script async src="svgs.js"><\/script>'/'<!--<script async src="svgs.js"><\/script>-->'/g index.html; fi
if [ $FOUND_PNGS -eq "0" ]; then sed -i.bak s/'<script async src="imgs.js"><\/script>'/'<!--<script async src="imgs.js"><\/script>-->'/g index.html; fi
if [ $FOUND_2XPNGS -eq "0" ]; then sed -i.bak s/'<script async src="imgs@2x.js"><\/script>'/'<!--<script async src="imgs@2x.js"><\/script>-->'/g index.html; fi

rm *.bak

# copy the WAM favicon
cp ../../../IPlug/WEB/Template/favicon.ico favicon.ico

cd ../

echo
echo MAKING  - WEB WASM MODULE -----------------------------

emmake make --makefile projects/$PROJECT_NAME-wam-controller.mk EXTRA_CFLAGS=-DWEBSOCKET_CLIENT=$WEBSOCKET_MODE

if [ $? -ne "0" ]; then
  echo IPlugWEB WASM compilation failed
  exit 1
fi

cd build-web

# print payload
echo payload:
find . -maxdepth 2 -mindepth 1 -exec du -hs {} \;
du -hc

# launch emrun
if [ "$LAUNCH_EMRUN" -eq "1" ]; then
  if [ "$EMRUN_SERVER" -eq "0" ]; then
    emrun --browser $EMRUN_BROWSER --no_server --port=$EMRUN_SERVER_PORT index.html
  else
    emrun --browser $EMRUN_BROWSER --no_emrun_detect index.html
  fi
else
  echo "Not running emrun"
fi
