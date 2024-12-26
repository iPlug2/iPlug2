#!/usr/bin/env bash

rm -r CLAP_SDK
rm -r CLAP_HELPERS

git clone https://github.com/free-audio/clap.git CLAP_SDK 
git clone https://github.com/free-audio/clap-helpers.git CLAP_HELPERS

git checkout ./CLAP_SDK/readme.txt
git checkout ./CLAP_HELPERS/readme.txt
