#!/bin/sh

if [ -d build-web ]
then
  rm -r build-web/scripts
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

cp ../../../../Dependencies/IPlug/WAM_SDK/wamsdk/*.js .
cp ../../../../Dependencies/IPlug/WAM_AWP/*.js .
cp ../../../../IPlug/WAM/Template/scripts/IPlugWAM-awn.js IPlugEffect-awn.js
sed -i "" s/IPlugWAM/IPlugEffect/g IPlugEffect-awn.js
cp ../../../../IPlug/WAM/Template/scripts/IPlugWAM-awp.js IPlugEffect-awp.js
sed -i "" s/IPlugWAM/IPlugEffect/g IPlugEffect-awp.js
cp ../../../../IPlug/WAM/Template/scripts/loader.js IPlugEffect-loader.js
cd ..
cp ../../../IPlug/WAM/Template/IPlugWAM-standalone.html IPlugEffect-standalone.html
sed -i "" s/IPlugWAM/IPlugEffect/g IPlugEffect-standalone.html

#python -m SimpleHTTPServer 8000 &

osascript <<EOF
tell application "Google Chrome Canary"
  open location "http://localhost:8000/IPlugEffect-standalone.html"
  activate
end tell
EOF
