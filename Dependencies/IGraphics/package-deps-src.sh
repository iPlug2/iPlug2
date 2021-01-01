#!/bin/sh

# This script is called on the CI/CD server in order to thin out unnessecary files from the built libs
# So that the zip downloads are a reasonable size

rm *.log
rm -r tmp
mv ./src/skia/include ./skia_include_tmp
mv ./src/skia/modules/skottie ./skia_skottie_tmp
mv ./src/skia/modules/particles ./skia_particles_tmp
mv ./src/skia/modules/skparagraph ./skia_skparagraph_tmp
mv ./src/skia/modules/skshaper ./skia_skshaper_tmp
mv ./src/skia/modules/skresources ./skia_skresources_tmp
mv ./src/skia/modules/svg ./skia_svg_tmp
mv ./src/skia/src/core ./skia_src_core_tmp
mv ./src/skia/src/xml ./skia_src_xml_tmp
mv ./src/skia/third_party/externals/icu/source/common/unicode ./skia_ext_icu_tmp

if [ "$(uname)" == "Darwin" ]; then
rm -r src
elif [ "$(expr substr $(uname -s) 1 5)" == "Linux" ]; then
rm -r src
else # on Windows we need to retain the entire contents of src since we don't have the unix folder heirarcy
rm -r src/skia
fi

mkdir -p ./src/skia
mkdir -p ./src/skia/src/
mkdir -p ./src/skia/modules
mkdir -p ./src/skia/third_party/externals/icu/source/common
mv ./skia_include_tmp ./src/skia/include
mv ./skia_skottie_tmp ./src/skia/modules/skottie
mv ./skia_particles_tmp ./src/skia/modules/particles
mv ./skia_skparagraph_tmp ./src/skia/modules/skparagraph
mv ./skia_skresources_tmp ./src/skia/modules/skresources
mv ./skia_skshaper_tmp ./src/skia/modules/skshaper
mv ./skia_svg_tmp ./src/skia/modules/svg
mv ./skia_src_core_tmp ./src/skia/src/core
mv ./skia_src_xml_tmp ./src/skia/src/xml
mv ./skia_ext_icu_tmp ./src/skia/third_party/externals/icu/source/common/unicode