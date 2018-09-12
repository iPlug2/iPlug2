#! /bin/sh

BASEDIR=$(dirname $0)

cd $BASEDIR/..

if [ -d build-mac ] 
then
  sudo rm -f -R -f build-mac
fi

#---------------------------------------------------------------------------------------------------------

#variables

DEMO=0
if [ "$1" == "demo" ]
then
DEMO=1
fi

VERSION=`echo | grep PLUG_VERSION_HEX config.h`
VERSION=${VERSION//\#define PLUG_VERSION_HEX }
VERSION=${VERSION//\'}
MAJOR_VERSION=$(($VERSION & 0xFFFF0000))
MAJOR_VERSION=$(($MAJOR_VERSION >> 16)) 
MINOR_VERSION=$(($VERSION & 0x0000FF00))
MINOR_VERSION=$(($MINOR_VERSION >> 8)) 
BUG_FIX=$(($VERSION & 0x000000FF))

FULL_VERSION=$MAJOR_VERSION"."$MINOR_VERSION"."$BUG_FIX

PLUGIN_NAME=`echo | grep BUNDLE_NAME config.h`
PLUGIN_NAME=${PLUGIN_NAME//\#define BUNDLE_NAME }
PLUGIN_NAME=${PLUGIN_NAME//\"}

DMG_NAME=$PLUGIN_NAME-v$FULL_VERSION-mac

if [ $DEMO == 1 ]
then
  DMG_NAME=$DMG_NAME-demo
fi


# work out the paths to the binaries

VST2=`echo | grep VST2_PATH ../../common-mac.xcconfig`
VST2=${VST2//\VST2_PATH = }/$PLUGIN_NAME.vst

VST3=`echo | grep VST3_PATH ../../common-mac.xcconfig`
VST3=${VST3//\VST3_PATH = }/$PLUGIN_NAME.vst3

AU=`echo | grep AU_PATH ../../common-mac.xcconfig`
AU=${AU//\AU_PATH = }/$PLUGIN_NAME.component

APP=`echo | grep APP_PATH ../../common-mac.xcconfig`
APP=${APP//\APP_PATH = }/$PLUGIN_NAME.app

# Dev build folder
AAX=`echo | grep AAX_PATH ../../common-mac.xcconfig`
AAX=${AAX//\AAX_PATH = }/$PLUGIN_NAME.aaxplugin
AAX_FINAL="/Library/Application Support/Avid/Audio/Plug-Ins/$PLUGIN_NAME.aaxplugin"

PKG="installer/build-mac/$PLUGIN_NAME Installer.pkg"
PKG_US="installer/build-mac/$PLUGIN_NAME Installer.unsigned.pkg"

CERT_ID=`echo | grep CERTIFICATE_ID ../../common-mac.xcconfig`
CERT_ID=${CERT_ID//\CERTIFICATE_ID = }

if [ $DEMO == 1 ]
then
  echo "making $PLUGIN_NAME version $FULL_VERSION DEMO mac distribution..."
#   cp "resources/img/AboutBox_Demo.png" "resources/img/AboutBox.png"
else
  echo "making $PLUGIN_NAME version $FULL_VERSION mac distribution..."
#   cp "resources/img/AboutBox_Registered.png" "resources/img/AboutBox.png"
fi

echo ""

#---------------------------------------------------------------------------------------------------------

./scripts/update_installer_version.py $DEMO

echo "touching source to force recompile" 
touch *.cpp

#---------------------------------------------------------------------------------------------------------

#remove existing dist folder
#if [ -d installer/dist ] 
#then
#  rm -R installer/dist
#fi

#mkdir installer/dist

#remove existing binaries
if [ -d $APP ] 
then
  sudo rm -f -R -f $APP
fi

if [ -d $AU ] 
then
  sudo rm -f -R $AU
fi

if [ -d $VST2 ] 
then
  sudo rm -f -R $VST2
fi

if [ -d $VST3 ] 
then
  sudo rm -f -R $VST3
fi

if [ -d "${AAX}" ] 
then
  sudo rm -f -R "${AAX}"
fi

if [ -d "${AAX_FINAL}" ] 
then
  sudo rm -f -R "${AAX_FINAL}"
fi

#---------------------------------------------------------------------------------------------------------

# build xcode project. Change target to build individual formats 

xcodebuild -project ./projects/$PLUGIN_NAME-macOS.xcodeproj -xcconfig ./config/$PLUGIN_NAME-mac.xcconfig DEMO_VERSION=$DEMO -target "All" -configuration Release 2> ./build-mac.log

if [ -s build-mac.log ]
then
  echo "build failed due to following errors:"
  echo ""
  cat build-mac.log
  exit 1
else
 rm build-mac.log
fi

#---------------------------------------------------------------------------------------------------------

#icon stuff - http://maxao.free.fr/telechargements/setfileicon.gz
echo "setting icons"
echo ""
setfileicon resources/$PLUGIN_NAME.icns $AU
setfileicon resources/$PLUGIN_NAME.icns $VST2
setfileicon resources/$PLUGIN_NAME.icns $VST3
setfileicon resources/$PLUGIN_NAME.icns "${AAX}"

#---------------------------------------------------------------------------------------------------------

#strip debug symbols from binaries

echo "stripping binaries"
strip -x $AU/Contents/Resources/plugin.vst3/Contents/MacOS/$PLUGIN_NAME
strip -x $VST2/Contents/MacOS/$PLUGIN_NAME
strip -x $VST3/Contents/MacOS/$PLUGIN_NAME
strip -x $APP/Contents/MacOS/$PLUGIN_NAME
strip -x "${AAX}/Contents/MacOS/$PLUGIN_NAME"

#---------------------------------------------------------------------------------------------------------

#ProTools stuff
echo "copying AAX ${PLUGIN_NAME} from 3PDev to main AAX folder"
sudo cp -p -R "${AAX}" "${AAX_FINAL}"
mkdir "${AAX_FINAL}/Contents/Factory Presets/"

echo "code sign AAX binary"
/Applications/PACEAntiPiracy/Eden/Fusion/Current/bin/wraptool sign --verbose --account XXXX --wcguid XXXX --signid "Developer ID Application: ""${CERT_ID}" --in "${AAX_FINAL}" --out "${AAX_FINAL}"

#---------------------------------------------------------------------------------------------------------

#Mac AppStore stuff

#xcodebuild -project $PLUGIN_NAME.xcodeproj -xcconfig $PLUGIN_NAME.xcconfig -target "APP" -configuration Release 2> ./build-mac.log

#echo "code signing app for appstore"
#echo ""
#codesign -f -s "3rd Party Mac Developer Application: ""${CERT_ID}" $APP --entitlements resources/$PLUGIN_NAME.entitlements
 
#echo "building pkg for app store"
#echo ""
#productbuild \
#     --component $APP /Applications \
#     --sign "3rd Party Mac Developer Installer: ""${CERT_ID}" \
#     --product "/Applications/$PLUGIN_NAME.app/Contents/Info.plist" installer/$PLUGIN_NAME.pkg

#---------------------------------------------------------------------------------------------------------

#10.8 Gatekeeper/Developer ID stuff

#echo "code app binary for Gatekeeper on 10.8"
#echo ""
#codesign -f -s "Developer ID Application: ""${CERT_ID}" $APP

#---------------------------------------------------------------------------------------------------------

# installer, uses Packages http://s.sudre.free.fr/Software/Packages/about.html
sudo sudo rm -R -f installer/$PLUGIN_NAME-mac.dmg

echo "building installer"
echo ""
packagesbuild installer/$PLUGIN_NAME.pkgproj

echo "code-sign installer for Gatekeeper on 10.8"
echo ""
mv "${PKG}" "${PKG_US}"
productsign --sign "Developer ID Installer: ""${CERT_ID}" "${PKG_US}" "${PKG}"
                   
rm -R -f "${PKG_US}"

#set installer icon
setfileicon resources/$PLUGIN_NAME.icns "${PKG}"

#---------------------------------------------------------------------------------------------------------

# dmg, can use dmgcanvas http://www.araelium.com/dmgcanvas/ to make a nice dmg

echo "building dmg"
echo ""

if [ -d installer/$PLUGIN_NAME.dmgCanvas ]
then
  dmgcanvas installer/$PLUGIN_NAME.dmgCanvas installer/$DMG_NAME.dmg
else
  hdiutil create installer/$DMG_NAME.dmg -format UDZO -srcfolder installer/build-mac/ -ov -anyowners -volname $PLUGIN_NAME
fi

sudo rm -R -f installer/build-mac/

#---------------------------------------------------------------------------------------------------------
# zip

# echo "copying binaries..."
# echo ""
# cp -R $AU installer/dist/$PLUGIN_NAME.component
# cp -R $VST2 installer/dist/$PLUGIN_NAME.vst
# cp -R $VST3 installer/dist/$PLUGIN_NAME.vst3
# cp -R $AAX installer/dist/$PLUGIN_NAME.aaxplugin
# cp -R $APP installer/dist/$PLUGIN_NAME.app
# 
# echo "zipping binaries..."
# echo ""
# ditto -c -k installer/dist installer/$PLUGIN_NAME-mac.zip
# rm -R installer/dist

#---------------------------------------------------------------------------------------------------------

if [ $DEMO == 1 ]
then
git checkout installer/IPlugEffect.iss
git checkout installer/IPlugEffect.pkgproj
git checkout resources/img/AboutBox.png
fi

echo "done"
