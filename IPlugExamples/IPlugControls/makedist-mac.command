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

VST2="/Library/Audio/Plug-Ins/VST/IPlugControls.vst"
VST3="/Library/Audio/Plug-Ins/VST3/IPlugControls.vst3"
APP="/Applications/IPlugControls.app"
AUDIOUNIT="/Library/Audio/Plug-Ins/Components/IPlugControls.component"
RTAS="/Library/Application Support/Digidesign/Plug-Ins/IPlugControls.dpm"

echo "making IPlugControls version $FULL_VERSION mac distribution..."
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

xcodebuild -project IPlugControls.xcodeproj -xcconfig IPlugControls.xcconfig -target "All" -configuration Release
#xcodebuild -project IPlugControls-ios.xcodeproj -xcconfig IPlugControls.xcconfig -target "IOSAPP" -configuration Release

#icon stuff - http://maxao.free.fr/telechargements/setfileicon.gz
echo "setting icons"
echo ""
setfileicon resources/IPlugControls.icns $AUDIOUNIT
setfileicon resources/IPlugControls.icns $VST2
setfileicon resources/IPlugControls.icns $VST3
setfileicon resources/IPlugControls.icns "${RTAS}"

#appstore stuff

# echo "code signing app"
# echo ""
# codesign -f -s "3rd Party Mac Developer Application: Oliver Larkin" $APP
#  
# echo "building pkg for app store"
# productbuild \
#      --component $APP /Applications \
#      --sign "3rd Party Mac Developer Installer: Oliver Larkin" \
#      --product "/Applications/IPlugControls.app/Contents/Info.plist" installer/IPlugControls.pkg

# installer, uses iceberg http://s.sudre.free.fr/Software/Iceberg.html

sudo sudo rm -R -f installer/IPlugControls-mac.dmg

echo "building installer"
echo ""
freeze installer/IPlugControls.packproj

# dmg, uses dmgcanvas http://www.araelium.com/dmgcanvas/

echo "building dmg"
echo ""

if [ -d installer/IPlugControls.dmgCanvas ]
then
  dmgcanvas installer/IPlugControls.dmgCanvas installer/IPlugControls-mac.dmg
else
  hdiutil create installer/IPlugControls.dmg -srcfolder installer/build-mac/ -ov -anyowners -volname IPlugControls
  
  if [ -f installer/IPlugControls-mac.dmg ]
  then
   rm -f installer/IPlugControls-mac.dmg
  fi
  
  hdiutil convert installer/IPlugControls.dmg -format UDZO -o installer/IPlugControls-mac.dmg
  sudo sudo rm -R -f installer/IPlugControls.dmg
fi

sudo sudo rm -R -f installer/build-mac/

# echo "copying binaries..."
# echo ""
# cp -R $AUDIOUNIT installer/dist/IPlugControls.component
# cp -R $VST2 installer/dist/IPlugControls.vst
# cp -R $VST3 installer/dist/IPlugControls.vst3
# cp -R $RTAS installer/dist/IPlugControls.dpm
# cp -R $APP installer/dist/IPlugControls.app
# 
# echo "zipping binaries..."
# echo ""
# ditto -c -k installer/dist installer/IPlugControls-mac.zip
# rm -R installer/dist

echo "done"