#!/bin/bash
#
# Copyright (c) 2017 Ollix
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# ---
# Author: olliwang@ollix.com (Olli Wang)
#
# This script compiles the metal source file "nanovg_mtl_shaders.metal"
# into metallib binary data as defined in "nanovg_mtl_metallib_*.h" headers.

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/../src

result=true

PREFIX="mnvg_bitcode"

mkdir $PREFIX

execute_cmd() {
  SDK=$1
  TARGET=$2

  SUFFIX=""
  CMD="xcrun -sdk $SDK metal -c nanovg_mtl_shaders.metal"

  AIR_FILE_NAME="$PREFIX/$TARGET$SUFFIX.air"
  HEADER_NAME="$PREFIX/$TARGET$SUFFIX.h"
  VARIABLE_NAME="$PREFIX/$TARGET$SUFFIX"

  if [ $SDK = "iphoneos" ]; then
    CMD="$CMD -mios-version-min=8.0"
  elif [ $SDK = "macosx" ]; then
    CMD="$CMD -mmacosx-version-min=10.11"
  fi

  CMD="$CMD -o $AIR_FILE_NAME"
  eval $CMD

  if [ $? -ne 0 ]; then
    return 1
  fi
  xcrun -sdk $SDK metallib $AIR_FILE_NAME -o $VARIABLE_NAME \
      > /dev/null 2>&1
  xxd -i $VARIABLE_NAME > $HEADER_NAME
  rm $AIR_FILE_NAME $VARIABLE_NAME
  return 0
}

for SDK in "iphoneos" "iphonesimulator" "macosx" "appletvos"; do
  if [ $SDK = "appletvos" ]; then
    echo "* Compiling for tvOS..."
    TARGET="tvos"
    execute_cmd $SDK $TARGET
    if [ $? -ne 0 ]; then
      result=false
      break
    fi
  else
    if [ $SDK = "iphoneos" ]; then
      echo "* Compiling for iOS..."
      TARGET="ios"
    elif [ $SDK = "iphonesimulator" ]; then
      echo "* Compiling for Simulator..."
      TARGET="simulator"
    elif [ $SDK = "macosx" ]; then
      echo "* Compiling for macOS..."
      TARGET="macos"
    fi

    execute_cmd $SDK $TARGET
    if [ $? -ne 0 ]; then
      result=false
      break
    fi
  fi
done

if $result; then
  echo "Done"
fi
