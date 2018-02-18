#! /bin/sh

BASEDIR=$(dirname $0)

cd $BASEDIR

echo "zipping plugins..."
b=$(date +%d_%m_%Y)

zip -r wdl-ol-$b.zip . -x "AAX_SDK/*" "PT9_SDK/*" "VST3_SDK/*" ".gitignore" "*/build-*" "*/.git/*" "old/*" "backup.command" "*/dist/*" "*.suo" "*.DS_Store" "*.dmg" "*.ncb" "*.zip" "*.pkg" "*.sdf"
zip -r wdl-ol-$b.zip "AAX_SDK/readme.txt" "PT9_SDK/readme.txt" "ASIO_SDK/readme.txt" "VST_SDK/readme.txt" "VST3_SDK/readme.txt"

echo "done"
