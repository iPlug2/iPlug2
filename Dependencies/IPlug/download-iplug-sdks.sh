#!/usr/bin/env bash

rm -r VST3_SDK
rm -r WAM_AWP
rm -r WAM_SDK
git clone https://github.com/iplug2/audioworklet-polyfill WAM_AWP
git clone https://github.com/iplug2/api.git WAM_SDK

git clone https://github.com/steinbergmedia/vst3sdk.git VST3_SDK
cd VST3_SDK
git submodule update --init pluginterfaces
git submodule update --init base
git submodule update --init public.sdk
git submodule update --init doc
cd ..
git checkout ./VST3_SDK/README.md
git checkout ./WAM_SDK/readme.txt
git checkout ./WAM_AWP/readme.txt