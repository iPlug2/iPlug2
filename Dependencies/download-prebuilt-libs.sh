#!/usr/bin/env bash

if [ "$1" == "" ]; then
  if [ "$(uname)" == "Darwin" ]; then
  ZIP_FILE=IPLUG2_DEPS_MAC
  FOLDER=mac
  elif [ "$(expr substr $(uname -s) 1 5)" == "Linux" ]; then
  exit
  else
  ZIP_FILE=IPLUG2_DEPS_WIN
  FOLDER=win
  fi
elif [ "$1" == "mac" ]; then
  ZIP_FILE=IPLUG2_DEPS_MAC
  FOLDER=mac
elif [ "$1" == "ios" ]; then
  ZIP_FILE=IPLUG2_DEPS_IOS
  FOLDER=ios
elif [ "$1" == "win" ]; then
  ZIP_FILE=IPLUG2_DEPS_WIN
  FOLDER=win
fi

curl https://github.com/iPlug2/iPlug2/releases/download/setup/$ZIP_FILE.zip -L -J -O

if [ ! -d Build ]; then 
  mkdir Build
fi

if [ -d Build/$FOLDER ]; then 
  rm -r Build/$FOLDER
fi

if [ -d Build/src ]; then 
  rm -r Build/src
fi

unzip -o $ZIP_FILE.zip
mv $ZIP_FILE/* Build


if [ "$FOLDER" == "win" ]; then
  curl https://github.com/iPlug2/iPlug2/releases/download/setup/IPLUG2_DEPS_WIN_FAUST.zip -L -J -O
  unzip -o IPLUG2_DEPS_WIN_FAUST.zip
  mkdir Build/win/Faust
  mv Faust/* Build/win/Faust
  rm -r Faust
fi


rm -r $ZIP_FILE
rm *.zip