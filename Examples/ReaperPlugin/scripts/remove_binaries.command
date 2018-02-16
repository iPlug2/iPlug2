#! /bin/sh

BASEDIR=$(dirname $0)

cd $BASEDIR

PLUGIN_NAME=`echo | grep BUNDLE_NAME ../config.h`
PLUGIN_NAME=${PLUGIN_NAME//\#define BUNDLE_NAME }
PLUGIN_NAME=${PLUGIN_NAME//\"}

# work out the paths to the binaries

VST2=`echo | grep VST_FOLDER ../../common.xcconfig`
VST2=${VST2//\VST_FOLDER = }/$PLUGIN_NAME.vst

VST3=`echo | grep VST3_FOLDER ../../common.xcconfig`
VST3=${VST3//\VST3_FOLDER = }/$PLUGIN_NAME.vst3

AU=`echo | grep AU_FOLDER ../../common.xcconfig`
AU=${AU//\AU_FOLDER = }/$PLUGIN_NAME.component

APP=`echo | grep APP_FOLDER ../../common.xcconfig`
APP=${APP//\APP_FOLDER = }/$PLUGIN_NAME.app

# Dev build folder
AAX=`echo | grep AAX_FOLDER ../../../common.xcconfig`
AAX=${AAX//\AAX_FOLDER = }/$PLUGIN_NAME.aaxplugin
AAX_FINAL="/Library/Application Support/Avid/Audio/Plug-Ins/$PLUGIN_NAME.aaxplugin"

if [ -d $VST2 ]
then
  sudo rm -r -f $VST2
fi

if [ -d $VST3 ]
then
  sudo rm -r -f $VST3
fi

if [ -d $AU ]
then
  sudo rm -r -f $AU
fi

if [ -d $AAX ]
then
  sudo rm -r -f $AAX
fi

if [ -d $APP ]
then
  sudo rm -r -f $APP
fi
