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

VST2="/Library/Audio/Plug-Ins/VST/IPlugMultiChannel.vst"
VST3="/Library/Audio/Plug-Ins/VST3/IPlugMultiChannel.vst3"
APP="/Applications/IPlugMultiChannel.app"
AUDIOUNIT="/Library/Audio/Plug-Ins/Components/IPlugMultiChannel.component"
RTAS="/Library/Application Support/Digidesign/Plug-Ins/IPlugMultiChannel.dpm"

echo "making IPlugMultiChannel version $FULL_VERSION mac distribution..."
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
  sudo rm -R -f $APP
fi

#remove existing AU
if [ -d $AUDIOUNIT ] 
then
  sudo rm -R $AUDIOUNIT
fi

#remove existing VST2
if [ -d $VST2 ] 
then
  sudo rm -R $VST2
fi

#remove existing VST3
if [ -d $VST3 ] 
then
  rm -R $VST3
fi


#remove existing RTAS
if [ -d "${RTAS}" ] 
then
  sudo rm -R "${RTAS}"
fi

xcodebuild -project IPlugMultiChannel.xcodeproj -xcconfig IPlugMultiChannel.xcconfig -target "All" -configuration Release
#xcodebuild -project IPlugMultiChannel-ios.xcodeproj -xcconfig IPlugMultiChannel.xcconfig -target "IOSAPP" -configuration Release

#icon stuff - http://maxao.free.fr/telechargements/setfileicon.gz
echo "setting icons"
echo ""
setfileicon resources/IPlugMultiChannel.icns $AUDIOUNIT
setfileicon resources/IPlugMultiChannel.icns $VST2
setfileicon resources/IPlugMultiChannel.icns $VST3
setfileicon resources/IPlugMultiChannel.icns "${RTAS}"

#appstore stuff

# echo "code signing app"
# echo ""
# codesign -f -s "3rd Party Mac Developer Application: Oliver Larkin" $APP
#  
# echo "building pkg for app store"
# productbuild \
#      --component $APP /Applications \
#      --sign "3rd Party Mac Developer Installer: Oliver Larkin" \
#      --product "/Applications/IPlugMultiChannel.app/Contents/Info.plist" installer/IPlugMultiChannel.pkg

# installer, uses iceberg http://s.sudre.free.fr/Software/Iceberg.html

sudo sudo rm -R -f installer/IPlugMultiChannel-mac.dmg

echo "building installer"
echo ""
freeze installer/IPlugMultiChannel.packproj

# dmg, uses dmgcanvas http://www.araelium.com/dmgcanvas/

echo "building dmg"
echo ""

if [ -d installer/IPlugMultiChannel.dmgCanvas ]
then
  dmgcanvas installer/IPlugMultiChannel.dmgCanvas installer/IPlugMultiChannel-mac.dmg
else
  hdiutil create installer/IPlugMultiChannel.dmg -srcfolder installer/build-mac/ -ov -anyowners -volname IPlugMultiChannel
  
  if [ -f installer/IPlugMultiChannel-mac.dmg ]
  then
   rm -f installer/IPlugMultiChannel-mac.dmg
  fi
  
  hdiutil convert installer/IPlugMultiChannel.dmg -format UDZO -o installer/IPlugMultiChannel-mac.dmg
  sudo sudo rm -R -f installer/IPlugMultiChannel.dmg
fi

sudo sudo rm -R -f installer/build-mac/

# echo "copying binaries..."
# echo ""
# cp -R $AUDIOUNIT installer/dist/IPlugMultiChannel.component
# cp -R $VST2 installer/dist/IPlugMultiChannel.vst
# cp -R $VST3 installer/dist/IPlugMultiChannel.vst3
# cp -R $RTAS installer/dist/IPlugMultiChannel.dpm
# cp -R $APP installer/dist/IPlugMultiChannel.app
# 
# echo "zipping binaries..."
# echo ""
# ditto -c -k installer/dist installer/IPlugMultiChannel-mac.zip
# rm -R installer/dist

echo "done"