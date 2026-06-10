#!/usr/bin/env bash
set -eo pipefail

# 1st argument = tag name for CLAP SDK (e.g., "1.2.5")
# 2nd argument = tag name for CLAP helpers (optional, defaults to main)

CLAP_TAG="main"
HELPERS_TAG="main"

if [ "$1" != "" ]; then
  CLAP_TAG=$1
fi

if [ "$2" != "" ]; then
  HELPERS_TAG=$2
fi

rm -f -r CLAP_SDK
rm -f -r CLAP_HELPERS

git clone https://github.com/free-audio/clap.git --branch $CLAP_TAG --single-branch --depth=1 CLAP_SDK
git clone https://github.com/free-audio/clap-helpers.git --branch $HELPERS_TAG --single-branch --depth=1 CLAP_HELPERS

rm -rf CLAP_SDK/.git CLAP_HELPERS/.git
git checkout ./CLAP_SDK/readme.txt
git checkout ./CLAP_HELPERS/readme.txt
