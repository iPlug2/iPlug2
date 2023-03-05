#!/usr/bin/env bash

sudo rm -r WAM_AWP
sudo rm -r WAM_SDK
git clone https://github.com/iplug2/audioworklet-polyfill WAM_AWP
git clone https://github.com/iplug2/api.git WAM_SDK
sudo rm -r ./WAM_AWP/.git
sudo rm -r ./WAM_SDK/.git
git checkout ./WAM_SDK/readme.txt
git checkout ./WAM_AWP/readme.txt

