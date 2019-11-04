#!/usr/bin/env bash
set -e

#should be executed in Extras/civetweb
DEPS_DIR="$PWD"
BUILD_DIR="$PWD/../../Build"
SRC_DIR="$BUILD_DIR/src"
TMP_DIR="$BUILD_DIR/tmp"
REPO_DIR="$TMP_DIR/civetweb"
INSTALL_DIR="$BUILD_DIR/mac"

if [ -d "$REPO_DIR" ]
then
  echo civetweb repo exists
else
  git clone https://github.com/civetweb/civetweb.git $REPO_DIR
fi
cd $REPO_DIR

export MACOSX_DEPLOYMENT_TARGET=10.9
make lib WITH_CPP=1 WITH_WEBSOCKET=1

if [ ! -d "$INSTALL_DIR/lib" ];
then
  mkdir -p $INSTALL_DIR/lib
fi

if [ ! -d "$INSTALL_DIR/include" ];
then
  mkdir -p $INSTALL_DIR/include
fi

cp libcivetweb.a $INSTALL_DIR/lib/libcivetweb.a
cp include/* $INSTALL_DIR/include