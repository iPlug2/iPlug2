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

VST2="/Library/Audio/Plug-Ins/VST/IPlugGUIResize.vst"
VST3="/Library/Audio/Plug-Ins/VST3/IPlugGUIResize.vst3"
APP="/Applications/IPlugGUIResize.app"
AUDIOUNIT="/Library/Audio/Plug-Ins/Components/IPlugGUIResize.component"
RTAS="/Library/Application Support/Digidesign/Plug-Ins/IPlugGUIResize.dpm"

echo "making IPlugGUIResize version $FULL_VERSION mac distribution..."
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

xcodebuild -project IPlugGUIResize.xcodeproj -xcconfig IPlugGUIResize.xcconfig -target "All" -configuration Release
#xcodebuild -project IPlugGUIResize-ios.xcodeproj -xcconfig IPlugGUIResize.xcconfig -target "IOSAPP" -configuration Release

#icon stuff - http://maxao.free.fr/telechargements/setfileicon.gz
echo "setting icons"
echo ""
setfileicon resources/IPlugGUIResize.icns $AUDIOUNIT
setfileicon resources/IPlugGUIResize.icns $VST2
setfileicon resources/IPlugGUIResize.icns $VST3
setfileicon resources/IPlugGUIResize.icns "${RTAS}"

#appstore stuff

# echo "code signing app"
# echo ""
# codesign -f -s "3rd Party Mac Developer Application: Oliver Larkin" $APP
#  
# echo "building pkg for app store"
# productbuild \
#      --component $APP /Applications \
#      --sign "3rd Party Mac Developer Installer: Oliver Larkin" \
#      --product "/Applications/IPlugGUIResize.app/Contents/Info.plist" installer/IPlugGUIResize.pkg

# installer, uses iceberg http://s.sudre.free.fr/Software/Iceberg.html

sudo sudo rm -R -f installer/IPlugGUIResize-mac.dmg

echo "building installer"
echo ""
freeze installer/IPlugGUIResize.packproj

# dmg, uses dmgcanvas http://www.araelium.com/dmgcanvas/

echo "building dmg"
echo ""

if [ -d installer/IPlugGUIResize.dmgCanvas ]
then
  dmgcanvas installer/IPlugGUIResize.dmgCanvas installer/IPlugGUIResize-mac.dmg
else
  hdiutil create installer/IPlugGUIResize.dmg -srcfolder installer/build-mac/ -ov -anyowners -volname IPlugGUIResize
  
  if [ -f installer/IPlugGUIResize-mac.dmg ]
  then
   rm -f installer/IPlugGUIResize-mac.dmg
  fi
  
  hdiutil convert installer/IPlugGUIResize.dmg -format UDZO -o installer/IPlugGUIResize-mac.dmg
  sudo sudo rm -R -f installer/IPlugGUIResize.dmg
fi

sudo sudo rm -R -f installer/build-mac/

# echo "copying binaries..."
# echo ""
# cp -R $AUDIOUNIT installer/dist/IPlugGUIResize.component
# cp -R $VST2 installer/dist/IPlugGUIResize.vst
# cp -R $VST3 installer/dist/IPlugGUIResize.vst3
# cp -R $RTAS installer/dist/IPlugGUIResize.dpm
# cp -R $APP installer/dist/IPlugGUIResize.app
# 
# echo "zipping binaries..."
# echo ""
# ditto -c -k installer/dist installer/IPlugGUIResize-mac.zip
# rm -R installer/dist

echo "done"