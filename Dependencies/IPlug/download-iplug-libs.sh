#!/usr/bin/env bash

rm -r WAM_AWP
git clone https://github.com/iplug2/audioworklet-polyfill WAM_AWP

rm -r WAM_SDK
git clone https://github.com/iplug2/api.git WAM_SDK

rm -r VST3_SDK
git clone -v --recurse-submodules https://github.com/steinbergmedia/vst3sdk.git VST3_SDK
