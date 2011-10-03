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

VST2="/Library/Audio/Plug-Ins/VST/IPlugEffect.vst"
VST3="/Library/Audio/Plug-Ins/VST3/IPlugEffect.vst3"
APP="/Applications/IPlugEffect.app"
AUDIOUNIT="/Library/Audio/Plug-Ins/Components/IPlugEffect.component"
RTAS="/Applications/Digidesign/ProTools_902_3PDev/ProTools_3PDev/Plug-Ins/IPlugEffect.dpm"

echo "making IPlugEffect version $FULL_VERSION mac distribution..."
echo ""

./update_version.py

#remove existing dist folder
#if [ -d installer/dist ] 
#then
#  rm -R installer/dist
#fi

#mkdir installer/dist

#remove existing App
if [ -d $APP ] 
then
  rm -R -f $APP
fi

#remove existing AU
if [ -d $AUDIOUNIT ] 
then
  rm -R $AUDIOUNIT
fi

#remove existing VST2
if [ -d $VST2 ] 
then
  rm -R $VST2
fi

#remove existing VST3
if [ -d $VST3 ] 
then
  rm -R $VST3
fi


#remove existing RTAS
if [ -d $RTAS ] 
then
  rm -R $RTAS
fi

xcodebuild -project IPlugEffect.xcodeproj -xcconfig IPlugEffect.xcconfig -target "All" -configuration Release
#xcodebuild -project IPlugEffect-ios.xcodeproj -xcconfig IPlugEffect.xcconfig -target "IOSAPP" -configuration Release

#icon stuff - http://maxao.free.fr/telechargements/setfileicon.gz
echo "setting icons"
echo ""
setfileicon resources/IPlugEffect.icns $AUDIOUNIT
setfileicon resources/IPlugEffect.icns $VST2
setfileicon resources/IPlugEffect.icns $VST3
setfileicon resources/IPlugEffect.icns $RTAS

#appstore stuff

echo "code signing app"
echo ""
codesign -f -s "3rd Party Mac Developer Application: Oliver Larkin" $APP
 
echo "building pkg for app store"
productbuild \
     --component $APP /Applications \
     --sign "3rd Party Mac Developer Installer: Oliver Larkin" \
     --product "/Applications/IPlugEffect.app/Contents/Info.plist" installer/IPlugEffect.pkg

# installer, uses iceberg http://s.sudre.free.fr/Software/Iceberg.html

rm -R -f installer/IPlugEffect-mac.dmg

if [ -d "/Library/Application Support/Digidesign/Plug-Ins/IPlugEffect.dpm" ] 
then
  rm -R "/Library/Application Support/Digidesign/Plug-Ins/IPlugEffect.dpm"
fi

cp -R $RTAS "/Library/Application Support/Digidesign/Plug-Ins/IPlugEffect.dpm"

echo "building installer"
echo ""
freeze installer/IPlugEffect.packproj

# dmg, uses dmgcanvas http://www.araelium.com/dmgcanvas/

echo "building dmg"
echo ""

if [ -d installer/IPlugEffect.dmgCanvas ]
then
  dmgcanvas installer/IPlugEffect.dmgCanvas installer/IPlugEffect-mac_v$FULL_VERSION.dmg
else
  hdiutil create installer/IPlugEffect.dmg -srcfolder installer/build-mac/ -ov -anyowners -volname IPlugEffect
  
  if [ -f installer/IPlugEffect-mac_v$FULL_VERSION.dmg ]
  then
   rm -f installer/IPlugEffect-mac_v$FULL_VERSION.dmg
  fi
  
  hdiutil convert installer/IPlugEffect.dmg -format UDZO -o installer/IPlugEffect-mac_v$FULL_VERSION.dmg
  rm -R -f installer/IPlugEffect.dmg
fi

rm -R -f installer/build-mac/

# echo "copying binaries..."
# echo ""
# cp -R $AUDIOUNIT installer/dist/IPlugEffect.component
# cp -R $VST2 installer/dist/IPlugEffect.vst
# cp -R $VST3 installer/dist/IPlugEffect.vst3
# cp -R $RTAS installer/dist/IPlugEffect.dpm
# cp -R $APP installer/dist/IPlugEffect.app
# 
# echo "zipping binaries..."
# echo ""
# ditto -c -k installer/dist installer/IPlugEffect-mac.zip
# rm -R installer/dist

echo "done"