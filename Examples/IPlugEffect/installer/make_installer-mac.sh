#!/bin/bash

# IPlug2 project macOS installer build script, using pkgbuild and productbuild
# based on script for SURGE https://github.com/surge-synthesizer/surge

# Documentation for pkgbuild and productbuild: https://developer.apple.com/library/archive/documentation/DeveloperTools/Reference/DistributionDefinitionRef/Chapters/Distribution_XML_Ref.html

# preflight check

if [[ ! -f ./make_installer-mac.sh ]]; then
	echo "You must run this script from within the installer directory!"
	exit 1
fi

# version
if [ "$PLUGIN_VERSION" != "" ]; then
    VERSION="$PLUGIN_VERSION"
elif [ "$1" != "" ]; then
    VERSION="$1"
fi

if [ "$VERSION" == "" ]; then
    echo "You must specify the version you are packaging!"
    echo "eg: ./make_installer-mac.sh 1.0.6"
    exit 1
fi

# locations
PRODUCTS="../build-mac"

VST2="IPlugEffect.vst"
VST3="IPlugEffect.vst3"
AU="IPlugEffect.component"
APP="IPlugEffect.app"
AAX="IPlugEffect.aaxplugin"

RSRCS="/Users/oli/Music/IPlugEffect/Resources"

OUTPUT_BASE_FILENAME="IPlugEffect Installer.pkg"

TARGET_DIR="build-mac"

build_flavor()
{
    TMPDIR=${TARGET_DIR}/tmp-pkg
    flavor=$1
    flavorprod=$2
    ident=$3
    loc=$4

    echo --- BUILDING IPlugEffect_${flavor}.pkg ---

    mkdir -p $TMPDIR
    cp -r $PRODUCTS/$flavorprod $TMPDIR

    pkgbuild --root $TMPDIR --identifier $ident --version $VERSION --install-location $loc IPlugEffect_${flavor}.pkg || exit 1

    rm -r $TMPDIR
}

# try to build VST2 package
if [[ -d $PRODUCTS/$VST2 ]]; then
    build_flavor "VST2" $VST2 "com.AcmeInc.vst2.pkg.IPlugEffect" "/Library/Audio/Plug-Ins/VST"
fi

# # try to build VST3 package
if [[ -d $PRODUCTS/$VST3 ]]; then
    build_flavor "VST3" $VST3 "com.AcmeInc.vst3.pkg.IPlugEffect" "/Library/Audio/Plug-Ins/VST3"
fi

# # try to build AU package
if [[ -d $PRODUCTS/$AU ]]; then
    build_flavor "AU" $AU "com.AcmeInc.au.pkg.IPlugEffect" "/Library/Audio/Plug-Ins/Components"
fi

# # try to build AAX package
if [[ -d $PRODUCTS/$AAX ]]; then
    build_flavor "AAX" $AAX "com.AcmeInc.aax.pkg.IPlugEffect" ""/Library/Application Support/Avid/Audio/Plug-Ins""
fi

# try to build App package
if [[ -d $PRODUCTS/$APP ]]; then
    build_flavor "APP" $APP "com.AcmeInc.app.pkg.IPlugEffect" "/Applications"
fi

# write build info to resources folder

echo "Version: ${VERSION}" > "$RSRCS/BuildInfo.txt"
echo "Packaged on: $(date "+%Y-%m-%d %H:%M:%S")" >> "$RSRCS/BuildInfo.txt"
echo "On host: $(hostname)" >> "$RSRCS/BuildInfo.txt"
echo "Commit: $(git rev-parse HEAD)" >> "$RSRCS/BuildInfo.txt"

# build resources package
# --scripts ResourcesPackageScript
pkgbuild --root "$RSRCS" --identifier "com.AcmeInc.resources.pkg.IPlugEffect" --version $VERSION --install-location "/tmp/IPlugEffect" IPlugEffect_RES.pkg

# remove build info from resource folder
rm "$RSRCS/BuildInfo.txt"

# create distribution.xml

if [[ -d $PRODUCTS/$VST2 ]]; then
	VST2_PKG_REF='<pkg-ref id="com.AcmeInc.vst2.pkg.IPlugEffect"/>'
	VST2_CHOICE='<line choice="com.AcmeInc.vst2.pkg.IPlugEffect"/>'
	VST2_CHOICE_DEF="<choice id=\"com.AcmeInc.vst2.pkg.IPlugEffect\" visible=\"true\" start_selected=\"true\" title=\"VST2 Plug-in\"><pkg-ref id=\"com.AcmeInc.vst2.pkg.IPlugEffect\"/></choice><pkg-ref id=\"com.AcmeInc.vst2.pkg.IPlugEffect\" version=\"${VERSION}\" onConclusion=\"none\">IPlugEffect_VST2.pkg</pkg-ref>"
fi
if [[ -d $PRODUCTS/$VST3 ]]; then
	VST3_PKG_REF='<pkg-ref id="com.AcmeInc.vst3.pkg.IPlugEffect"/>'
	VST3_CHOICE='<line choice="com.AcmeInc.vst3.pkg.IPlugEffect"/>'
	VST3_CHOICE_DEF="<choice id=\"com.AcmeInc.vst3.pkg.IPlugEffect\" visible=\"true\" start_selected=\"true\" title=\"VST3 Plug-in\"><pkg-ref id=\"com.AcmeInc.vst3.pkg.IPlugEffect\"/></choice><pkg-ref id=\"com.AcmeInc.vst3.pkg.IPlugEffect\" version=\"${VERSION}\" onConclusion=\"none\">IPlugEffect_VST3.pkg</pkg-ref>"
fi
if [[ -d $PRODUCTS/$AU ]]; then
	AU_PKG_REF='<pkg-ref id="com.AcmeInc.au.pkg.IPlugEffect"/>'
	AU_CHOICE='<line choice="com.AcmeInc.au.pkg.IPlugEffect"/>'
	AU_CHOICE_DEF="<choice id=\"com.AcmeInc.au.pkg.IPlugEffect\" visible=\"true\" start_selected=\"true\" title=\"Audio Unit (v2) Plug-in\"><pkg-ref id=\"com.AcmeInc.au.pkg.IPlugEffect\"/></choice><pkg-ref id=\"com.AcmeInc.au.pkg.IPlugEffect\" version=\"${VERSION}\" onConclusion=\"none\">IPlugEffect_AU.pkg</pkg-ref>"
fi
if [[ -d $PRODUCTS/$AAX ]]; then
	AAX_PKG_REF='<pkg-ref id="com.AcmeInc.aax.pkg.IPlugEffect"/>'
	AAX_CHOICE='<line choice="com.AcmeInc.aax.pkg.IPlugEffect"/>'
	AAX_CHOICE_DEF="<choice id=\"com.AcmeInc.aax.pkg.IPlugEffect\" visible=\"true\" start_selected=\"true\" title=\"AAX Plug-in\"><pkg-ref id=\"com.AcmeInc.aax.pkg.IPlugEffect\"/></choice><pkg-ref id=\"com.AcmeInc.aax.pkg.IPlugEffect\" version=\"${VERSION}\" onConclusion=\"none\">IPlugEffect_AAX.pkg</pkg-ref>"
fi
if [[ -d $PRODUCTS/$APP ]]; then
	APP_PKG_REF='<pkg-ref id="com.AcmeInc.app.pkg.IPlugEffect"/>'
	APP_CHOICE='<line choice="com.AcmeInc.app.pkg.IPlugEffect"/>'
	APP_CHOICE_DEF="<choice id=\"com.AcmeInc.app.pkg.IPlugEffect\" visible=\"true\" start_selected=\"true\" title=\"Stand-alone App\"><pkg-ref id=\"com.AcmeInc.app.pkg.IPlugEffect\"/></choice><pkg-ref id=\"com.AcmeInc.app.pkg.IPlugEffect\" version=\"${VERSION}\" onConclusion=\"none\">IPlugEffect_APP.pkg</pkg-ref>"
fi

# if [[ -d $PRODUCTS/$RES ]]; then
	RES_PKG_REF='<pkg-ref id="com.AcmeInc.resources.pkg.IPlugEffect"/>'
	RES_CHOICE='<line choice="com.AcmeInc.resources.pkg.IPlugEffect"/>'
	RES_CHOICE_DEF="<choice id=\"com.AcmeInc.resources.pkg.IPlugEffect\" visible=\"true\" enabled=\"false\" selected=\"true\" title=\"Shared Resources\"><pkg-ref id=\"com.AcmeInc.resources.pkg.IPlugEffect\"/></choice><pkg-ref id=\"com.AcmeInc.resources.pkg.IPlugEffect\" version=\"${VERSION}\" onConclusion=\"none\">IPlugEffect_RES.pkg</pkg-ref>"
# fi

cat > distribution.xml << XMLEND
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="1">
    <title>IPlugEffect ${VERSION}</title>
    <license file="license.rtf" mime-type="application/rtf"/>
    <readme file="readme-mac.rtf" mime-type="application/rtf"/>
    <welcome file="intro.rtf" mime-type="application/rtf"/>
    <background file="IPlugEffect-installer-bg.png" alignment="topleft" scaling="none"/>
    ${VST2_PKG_REF}
    ${VST3_PKG_REF}
    ${AU_PKG_REF}
    ${AAX_PKG_REF}
    ${APP_PKG_REF}
    ${RES_PKG_REF}
    <options require-scripts="false" customize="always" />
    <choices-outline>
        ${VST2_CHOICE}
        ${VST3_CHOICE}
        ${AU_CHOICE}
        ${AAX_CHOICE}
        ${APP_CHOICE}
        ${RES_CHOICE}
    </choices-outline>
    ${VST2_CHOICE_DEF}
    ${VST3_CHOICE_DEF}
    ${AU_CHOICE_DEF}
    ${AAX_CHOICE_DEF}
    ${APP_CHOICE_DEF}
    ${RES_CHOICE_DEF}
</installer-gui-script>
XMLEND

# build installation bundle

if [[ ! -d ${TARGET_DIR} ]]; then
	mkdir ${TARGET_DIR}
fi
productbuild --distribution distribution.xml --package-path "./" --resources . "${TARGET_DIR}/$OUTPUT_BASE_FILENAME"

rm distribution.xml
rm IPlugEffect_*.pkg
