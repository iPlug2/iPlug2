#! /bin/sh

BASEDIR=$(dirname $0)

cd $BASEDIR

echo "zipping plugins..."
b=$(date +%d_%m_%Y)

zip -r wdl-ol-$b.zip . -x "PT9_SDK/*" "VST3_SDK/*" ".gitignore" "*/build-*" ".git/*" "old/*" "backup.command" "*/dist/*" "*.suo" "*.DS_Store" "*.dmg" "*.ncb" "*.zip" "*.pkg" "*.sdf"
zip -r wdl-ol-$b.zip "PT9_SDK/filestoputhere.txt" "ASIO_SDK/filestoputhere.txt" "VST_SDK/filestoputhere.txt"

echo "done"
