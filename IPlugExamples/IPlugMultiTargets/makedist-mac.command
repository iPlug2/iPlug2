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

VST2="/Library/Audio/Plug-Ins/VST/IPlugMultiTargets.vst"
VST3="/Library/Audio/Plug-Ins/VST3/IPlugMultiTargets.vst3"
APP="/Applications/IPlugMultiTargets.app"
AUDIOUNIT="/Library/Audio/Plug-Ins/Components/IPlugMultiTargets.component"
RTAS="/Library/Application Support/Digidesign/Plug-Ins/IPlugMultiTargets.dpm"

echo "making IPlugMultiTargets version $FULL_VERSION mac distribution..."
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
  sudo rm -R $VST3
fi

#remove existing RTAS
if [ -d "${RTAS}" ] 
then
  sudo rm -R "${RTAS}"
fi

xcodebuild -project IPlugMultiTargets.xcodeproj -xcconfig IPlugMultiTargets.xcconfig -target "All" -configuration Release
#xcodebuild -project IPlugMultiTargets-ios.xcodeproj -xcconfig IPlugMultiTargets.xcconfig -target "IOSAPP" -configuration Release

#icon stuff - http://maxao.free.fr/telechargements/setfileicon.gz
echo "setting icons"
echo ""
setfileicon resources/IPlugMultiTargets.icns $AUDIOUNIT
setfileicon resources/IPlugMultiTargets.icns $VST2
setfileicon resources/IPlugMultiTargets.icns $VST3
setfileicon resources/IPlugMultiTargets.icns "${RTAS}"

#appstore stuff

# echo "code signing app"
# echo ""
# codesign -f -s "3rd Party Mac Developer Application: Oliver Larkin" $APP
#  
# echo "building pkg for app store"
# productbuild \
#      --component $APP /Applications \
#      --sign "3rd Party Mac Developer Installer: Oliver Larkin" \
#      --product "/Applications/IPlugMultiTargets.app/Contents/Info.plist" installer/IPlugMultiTargets.pkg

# installer, uses iceberg http://s.sudre.free.fr/Software/Iceberg.html

rm -R -f installer/IPlugMultiTargets-mac.dmg

echo "building installer"
echo ""
freeze installer/IPlugMultiTargets.packproj

# dmg, uses dmgcanvas http://www.araelium.com/dmgcanvas/

echo "building dmg"
echo ""

if [ -d installer/IPlugMultiTargets.dmgCanvas ]
then
  dmgcanvas installer/IPlugMultiTargets.dmgCanvas installer/IPlugMultiTargets-mac.dmg
else
  hdiutil create installer/IPlugMultiTargets.dmg -srcfolder installer/build-mac/ -ov -anyowners -volname IPlugMultiTargets
  
  if [ -f installer/IPlugMultiTargets-mac.dmg ]
  then
   rm -f installer/IPlugMultiTargets-mac.dmg
  fi
  
  hdiutil convert installer/IPlugMultiTargets.dmg -format UDZO -o installer/IPlugMultiTargets-mac.dmg
  rm -R -f installer/IPlugMultiTargets.dmg
fi

rm -R -f installer/build-mac/

# echo "copying binaries..."
# echo ""
# cp -R $AUDIOUNIT installer/dist/IPlugMultiTargets.component
# cp -R $VST2 installer/dist/IPlugMultiTargets.vst
# cp -R $VST3 installer/dist/IPlugMultiTargets.vst3
# cp -R $RTAS installer/dist/IPlugMultiTargets.dpm
# cp -R $APP installer/dist/IPlugMultiTargets.app
# 
# echo "zipping binaries..."
# echo ""
# ditto -c -k installer/dist installer/IPlugMultiTargets-mac.zip
# rm -R installer/dist

echo "done"