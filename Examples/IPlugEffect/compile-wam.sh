#!/bin/sh

if [ -d build-web ]
then
  rm -r build-web/*
else
  mkdir build-web
fi

mkdir build-web/scripts
emmake make
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

mkdir wamsdk
mkdir awp
cp -r ../../../../Dependencies/IPlug/WAM_SDK/wamsdk wamsdk
cp ../../../../Dependencies/IPlug/WAM_AWP/*.js awp
cp ../../../../IPlug/WAM/Template/scripts/*.js .
cd ..
cp ../../../IPlug/WAM/Template/IPlugWAM-*.html .
python -m SimpleHTTPServer & open -n -a Google\ Chrome\ Canary http://localhost:8000/IPlugWAM-standalone.html