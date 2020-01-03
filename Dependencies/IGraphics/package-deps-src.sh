#!/bin/sh

# This script is called on the CI/CD server in order to thin out unnessecary files from the built libs
# So that the zip downloads are a reasonable size

rm *.log
rm -r tmp
mv ./src/skia/include ./skia_include_tmp
mv ./src/skia/modules ./skia_modules_tmp
mv ./src/skia/experimental/svg/model ./skia_experimental_tmp
mv ./src/skia/src/core ./skia_src_core_tmp
mv ./src/skia/src/xml ./skia_src_xml_tmp

if [ "$(uname)" == "Darwin" ]; then
rm -r src
elif [ "$(expr substr $(uname -s) 1 5)" == "Linux" ]; then
rm -r src
else # on Windows we need to retain the entire contents of src since we don't have the unix folder heirarcy
rm -r src/skia
fi

mkdir -p ./src/skia
mkdir -p ./src/skia/experimental/svg
mkdir -p ./src/skia/src/
mv ./skia_include_tmp ./src/skia/include
mv ./skia_modules_tmp ./src/skia/modules
mv ./skia_experimental_tmp ./src/skia/experimental/svg/model
mv ./skia_src_core_tmp ./src/skia/src/core
mv ./skia_src_xml_tmp ./src/skia/src/xml