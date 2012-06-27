#! /bin/sh

BASEDIR=$(dirname $0)

# AAX codesigning requires ashelper tool in /usr/local/bin and aax.key/.crt in ./../../../Certificates/


cd $BASEDIR

#---------------------------------------------------------------------------------------------------------

#variables

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
VST2=${VST2//\VST_FOLDER = }/IPlugMultiChannel.vst

VST3=`echo | grep VST3_FOLDER ../../common.xcconfig`
VST3=${VST3//\VST3_FOLDER = }/IPlugMultiChannel.vst3

AU=`echo | grep AU_FOLDER ../../common.xcconfig`
AU=${AU//\AU_FOLDER = }/IPlugMultiChannel.component

APP=`echo | grep APP_FOLDER ../../common.xcconfig`
APP=${APP//\APP_FOLDER = }/IPlugMultiChannel.app

# Dev build folder
RTAS=`echo | grep RTAS_FOLDER ../../common.xcconfig`
RTAS=${RTAS//\RTAS_FOLDER = }/IPlugMultiChannel.dpm
RTAS_FINAL="/Library/Application Support/Digidesign/Plug-Ins/IPlugMultiChannel.dpm"

# Dev build folder
AAX=`echo | grep AAX_FOLDER ../../common.xcconfig`
AAX=${AAX//\AAX_FOLDER = }/IPlugMultiChannel.aaxplugin
AAX_FINAL="/Library/Application Support/Avid/Audio/Plug-Ins/IPlugMultiChannel.aaxplugin"

PKG='installer/build-mac/IPlugMultiChannel Installer.pkg'
PKG_US='installer/build-mac/IPlugMultiChannel Installer.unsigned.pkg'

CERT_ID=`echo | grep CERTIFICATE_ID ../../common.xcconfig`
CERT_ID=${CERT_ID//\CERTIFICATE_ID = }

echo "making IPlugMultiChannel version $FULL_VERSION mac distribution..."
echo ""

#---------------------------------------------------------------------------------------------------------

./update_version.py

#could use touch to force a rebuild
#touch blah.h

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

if [ -d "${RTAS}" ] 
then
  sudo rm -f -R "${RTAS}"
fi

if [ -d "${RTAS_FINAL}" ] 
then
  sudo rm -f -R "${RTAS_FINAL}"
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
xcodebuild -project IPlugMultiChannel.xcodeproj -xcconfig IPlugMultiChannel.xcconfig -target "All" -configuration Release 2> ./build-mac.log
#xcodebuild -project IPlugMultiChannel-ios.xcodeproj -xcconfig IPlugMultiChannel.xcconfig -target "IOSAPP" -configuration Release

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
setfileicon resources/IPlugMultiChannel.icns $AU
setfileicon resources/IPlugMultiChannel.icns $VST2
setfileicon resources/IPlugMultiChannel.icns $VST3
setfileicon resources/IPlugMultiChannel.icns "${RTAS}"
setfileicon resources/IPlugMultiChannel.icns "${AAX}"

#---------------------------------------------------------------------------------------------------------

#ProTools stuff

echo "copying RTAS bundle from 3PDev to main RTAS folder"
sudo cp -p -R "${RTAS}" "${RTAS_FINAL}"

echo "copying AAX bundle from 3PDev to main AAX folder"
sudo cp -p -R "${AAX}" "${AAX_FINAL}"

echo "code sign AAX binary"
sudo ashelper -f "${AAX_FINAL}/Contents/MacOS/IPlugMultiChannel" -l ../../../Certificates/aax.crt -k ../../../Certificates/aax.key -o "${AAX_FINAL}/Contents/MacOS/IPlugMultiChannel"
#---------------------------------------------------------------------------------------------------------

#appstore stuff

# echo "code signing app for appstore"
# echo ""
# codesign -f -s "3rd Party Mac Developer Application: ""${CERT_ID}" $APP --entitlements resources/IPlugMultiChannel.entitlements
#  
# echo "building pkg for app store"
# productbuild \
#      --component $APP /Applications \
#      --sign "3rd Party Mac Developer Installer: ""${CERT_ID}" \
#      --product "/Applications/IPlugMultiChannel.app/Contents/Info.plist" installer/IPlugMultiChannel.pkg

#---------------------------------------------------------------------------------------------------------

#10.8 Gatekeeper/Developer ID stuff

#echo "code sign app for Gatekeeper on 10.8"
#echo ""
#codesign -f -s "Developer ID Application: ""${CERT_ID}" $APP

#---------------------------------------------------------------------------------------------------------

# installer, uses Packages http://s.sudre.free.fr/Software/Packages/about.html
sudo sudo rm -R -f installer/IPlugMultiChannel-mac.dmg

echo "building installer"
echo ""
packagesbuild installer/IPlugMultiChannel.pkgproj

#echo "code sign installer for Gatekeeper on 10.8"
#echo ""
#mv "${PKG}" "${PKG_US}"
#productsign --sign "Developer ID Installer: ""${CERT_ID}" "${PKG_US}" "${PKG}"
                   
#rm -R -f "${PKG_US}"

#set installer icon
setfileicon resources/IPlugMultiChannel.icns "${PKG}"

#---------------------------------------------------------------------------------------------------------

# dmg, can use dmgcanvas http://www.araelium.com/dmgcanvas/ to make a nice dmg

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
  sudo rm -R -f installer/IPlugMultiChannel.dmg
fi

sudo rm -R -f installer/build-mac/

#---------------------------------------------------------------------------------------------------------
# zip

# echo "copying binaries..."
# echo ""
# cp -R $AU installer/dist/IPlugMultiChannel.component
# cp -R $VST2 installer/dist/IPlugMultiChannel.vst
# cp -R $VST3 installer/dist/IPlugMultiChannel.vst3
# cp -R $RTAS installer/dist/IPlugMultiChannel.dpm
# cp -R $AAX installer/dist/IPlugMultiChannel.aaxplugin
# cp -R $APP installer/dist/IPlugMultiChannel.app
# 
# echo "zipping binaries..."
# echo ""
# ditto -c -k installer/dist installer/IPlugMultiChannel-mac.zip
# rm -R installer/dist

#---------------------------------------------------------------------------------------------------------

echo "done"