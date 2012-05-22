#! /bin/sh

BASEDIR=$(dirname $0)

cd $BASEDIR

VERSION=`echo | grep PLUG_VER resource.h`
VERSION=${VERSION//\#define PLUG_VER }
VERSION=${VERSION//\'}
MAJOR_VERSION=$(($VERSION & 0xFFFF0000))
MAJOR_VERSION=$(($MAJOR_VERSION >> 16)) 
MINOR_VERSION=$(($VERSION & 0x0000FF00))
MINOR_VERSION=$(($MINOR_VERSION >> 8)) 
BUG_FIX=$(($VERSION & 0x000000FF))

FULL_VERSION=$MAJOR_VERSION"."$MINOR_VERSION"."$BUG_FIX

# work out the paths to the bundles

VST2=`echo | grep VST_FOLDER ../../common.xcconfig`
VST2=${VST2//\VST_FOLDER = }/IPlugEffect.vst

VST3=`echo | grep VST3_FOLDER ../../common.xcconfig`
VST3=${VST3//\VST3_FOLDER = }/IPlugEffect.vst3

AU=`echo | grep AU_FOLDER ../../common.xcconfig`
AU=${AU//\AU_FOLDER = }/IPlugEffect.component

APP=`echo | grep APP_FOLDER ../../common.xcconfig`
APP=${APP//\APP_FOLDER = }/IPlugEffect.app

# Dev build folder
RTAS=`echo | grep RTAS_FOLDER ../../common.xcconfig`
RTAS=${RTAS//\RTAS_FOLDER = }/IPlugEffect.dpm

# Dev build folder
AAX=`echo | grep AAX_FOLDER ../../common.xcconfig`
AAX=${AAX//\AAX_FOLDER = }/IPlugEffect.aaxplugin

echo "making IPlugEffect version $FULL_VERSION mac distribution..."
echo ""

./update_version.py

#could use touch to force a rebuild
#touch blah.h

#remove existing dist folder
#if [ -d installer/dist ] 
#then
#  rm -R installer/dist
#fi

#mkdir installer/dist

#remove existing App
if [ -d $APP ] 
then
  sudo rm -f -R -f $APP
fi

#remove existing AU
if [ -d $AU ] 
then
  sudo rm -f -R $AU
fi

#remove existing VST2
if [ -d $VST2 ] 
then
  sudo rm -f -R $VST2
fi

#remove existing VST3
if [ -d $VST3 ] 
then
  rm -f -R $VST3
fi

#remove existing RTAS
if [ -d "${RTAS}" ] 
then
  sudo rm -f -R "${RTAS}"
fi

#remove existing AAX
if [ -d "${AAX}" ] 
then
  sudo rm -f -R "${AAX}"
fi

xcodebuild -project IPlugEffect.xcodeproj -xcconfig IPlugEffect.xcconfig -target "All" -configuration Release
#xcodebuild -project IPlugEffect-ios.xcodeproj -xcconfig IPlugEffect.xcconfig -target "IOSAPP" -configuration Release

#icon stuff - http://maxao.free.fr/telechargements/setfileicon.gz
echo "setting icons"
echo ""
setfileicon resources/IPlugEffect.icns $AU
setfileicon resources/IPlugEffect.icns $VST2
setfileicon resources/IPlugEffect.icns $VST3
setfileicon resources/IPlugEffect.icns "${RTAS}"
setfileicon resources/IPlugEffect.icns "${AAX}"

#ProTools stuff

echo "copying RTAS bundle from 3PDev to main RTAS folder"
sudo cp -R $RTAS "/Library/Application Support/Digidesign/Plug-Ins/IPlugEffect.dpm"
RTAS="/Library/Application Support/Digidesign/Plug-Ins/IPlugEffect.dpm"

echo "copying AAX bundle from 3PDev to main AAX folder"
sudo cp -R $AAX "/Library/Application Support/Avid/Audio/Plug-Ins/IPlugEffect.aaxplugin"
AAX="/Library/Application Support/Avid/Audio/Plug-Ins/IPlugEffect.aaxplugin"

echo "TODO: codesign AAX binary"

#appstore stuff

# echo "code signing app for appstore"
# echo ""
# codesign -f -s "3rd Party Mac Developer Application: Oliver Larkin" $APP
#  
# echo "building pkg for app store"
# productbuild \
#      --component $APP /Applications \
#      --sign "3rd Party Mac Developer Installer: Oliver Larkin" \
#      --product "/Applications/IPlugEffect.app/Contents/Info.plist" installer/IPlugEffect.pkg

#10.8 Gatekeeper/Developer ID stuff

#echo "code sign app for Gatekeeper on 10.8"
#echo ""
#codesign -f -s "Developer ID Application: Oliver Larkin" $APP

# installer, uses iceberg http://s.sudre.free.fr/Software/Iceberg.html

sudo sudo rm -R -f installer/IPlugEffect-mac.dmg

echo "building installer"
echo ""
freeze installer/IPlugEffect.packproj

#echo "code sign installer for Gatekeeper on 10.8"
#echo ""
#mkdir installer/build-mac/signed/
#this productsign doesn't seem to work
#productsign --sign "Developer ID Installer: Oliver Larkin" \
#                   "installer/build-mac/IPlugEffect Installer.mpkg" \
#                   "installer/build-mac/signed/IPlugEffect Installer.mpkg"

# dmg, can use dmgcanvas http://www.araelium.com/dmgcanvas/ to make a nice dmg

echo "building dmg"
echo ""

if [ -d installer/IPlugEffect.dmgCanvas ]
then
  dmgcanvas installer/IPlugEffect.dmgCanvas installer/IPlugEffect-mac.dmg
else
  hdiutil create installer/IPlugEffect.dmg -srcfolder installer/build-mac/ -ov -anyowners -volname IPlugEffect
  
  if [ -f installer/IPlugEffect-mac.dmg ]
  then
   rm -f installer/IPlugEffect-mac.dmg
  fi
  
  hdiutil convert installer/IPlugEffect.dmg -format UDZO -o installer/IPlugEffect-mac.dmg
  sudo rm -R -f installer/IPlugEffect.dmg
fi

sudo rm -R -f installer/build-mac/

# echo "copying binaries..."
# echo ""
# cp -R $AU installer/dist/IPlugEffect.component
# cp -R $VST2 installer/dist/IPlugEffect.vst
# cp -R $VST3 installer/dist/IPlugEffect.vst3
# cp -R $RTAS installer/dist/IPlugEffect.dpm
# cp -R $AAX installer/dist/IPlugEffect.aaxplugin
# cp -R $APP installer/dist/IPlugEffect.app
# 
# echo "zipping binaries..."
# echo ""
# ditto -c -k installer/dist installer/IPlugEffect-mac.zip
# rm -R installer/dist

echo "done"