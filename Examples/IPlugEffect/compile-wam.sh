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
let wasmOut = \"AudioWorkletGlobalScope.WAM = { ENVIRONMENT: 'WEB' }\\\n\";
wasmOut += \"AudioWorkletGlobalScope.WAM.wasmBinary = new Uint8Array([\" + wasmStr + \"]);\";
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
cp -r ../../../../Dependencies/IPlug/WAM_SDK/ wamsdk
cp -r ../../../../Dependencies/IPlug/WAM_AWP/ awp
cp ../../../../IPlug/WAM/Template/scripts/*.js .
cd ..
cp ../../../IPlug/WAM/Template/IPlugWAM-*.html .
