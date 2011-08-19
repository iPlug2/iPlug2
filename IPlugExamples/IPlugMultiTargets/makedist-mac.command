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

VST2="/Library/Audio/Plug-Ins/VST/IPlugMultiTargets.vst"
APP="/Applications/IPlugMultiTargets.app"
AUDIOUNIT="/Library/Audio/Plug-Ins/Components/IPlugMultiTargets.component"
RTAS="/Applications/Digidesign/ProTools_902_3PDev/ProTools_3PDev/Plug-Ins/IPlugMultiTargets.dpm"

echo "making IPlugMultiTargets version $MAJOR_VERSION.$MINOR_VERSION.$BUG_FIX mac distribution..."
echo ""

echo updating version numbers
echo ""
./update_version.py --major $MAJOR_VERSION --minor $MINOR_VERSION --bug $BUG_FIX

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

#remove existing VST
if [ -d $VST2 ] 
then
  rm -R $VST2
fi

#remove existing RTAS
if [ -d $RTAS ] 
then
  rm -R $RTAS
fi

xcodebuild -project IPlugMultiTargets.xcodeproj -xcconfig IPlugMultiTargets.xcconfig -target "All" -configuration Release
#xcodebuild -project IPlugMultiTargets-ios.xcodeproj -xcconfig IPlugMultiTargets.xcconfig -target "IOSAPP" -configuration Release

#icon stuff - http://maxao.free.fr/telechargements/setfileicon.gz
echo "setting icons"
echo ""
setfileicon resources/IPlugMultiTargets.icns $AUDIOUNIT
setfileicon resources/IPlugMultiTargets.icns $VST2
setfileicon resources/IPlugMultiTargets.icns $RTAS

#appstore stuff

echo "building plugins only installer"
echo ""
freeze installer/IPlugMultiTargets-plugins.packproj
mv installer/build-mac/install-plugins.pkg /Applications/IPlugMultiTargets.app/Contents/Resources/install-plugins.pkg
cp installer/changelog.txt /Applications/IPlugMultiTargets.app/Contents/Resources/changelog.txt

echo "code signing app"
echo ""
codesign -f -s "3rd Party Mac Developer Application: Oliver Larkin" $APP
 
echo "building pkg for app store"
productbuild \
     --component $APP /Applications \
     --sign "3rd Party Mac Developer Installer: Oliver Larkin" \
     --product "/Applications/IPlugMultiTargets.app/Contents/Info.plist" installer/IPlugMultiTargets.pkg

# installer, uses iceberg http://s.sudre.free.fr/Software/Iceberg.html

#rm -R -f /Applications/IPlugMultiTargets.app/Contents/Resources/install-plugins.pkg
rm -R -f installer/IPlugMultiTargets-mac.dmg

echo "building all installer"
echo ""
freeze installer/IPlugMultiTargets-all.packproj

# dmg, uses dmgcanvas http://www.araelium.com/dmgcanvas/

echo "building dmg"
echo ""
dmgcanvas installer/IPlugMultiTargets.dmgCanvas installer/IPlugMultiTargets-mac.dmg

rm -R -f installer/build-mac/

# echo "copying binaries..."
# echo ""
# cp -R $AUDIOUNIT installer/dist/IPlugMultiTargets.component
# cp -R $VST2 installer/dist/IPlugMultiTargets.vst
# cp -R $RTAS installer/dist/IPlugMultiTargets.dpm
# cp -R $APP installer/dist/IPlugMultiTargets.app
# 
# echo "zipping binaries..."
# echo ""
# ditto -c -k installer/dist installer/IPlugMultiTargets-mac.zip
# rm -R installer/dist

echo "done"