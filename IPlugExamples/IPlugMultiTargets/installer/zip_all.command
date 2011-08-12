BASEDIR=$(dirname $0)

cd $BASEDIR

VERSION=`echo | grep PLUG_VER ../resource.h`
VERSION=${VERSION//\#define PLUG_VER }
VERSION=${VERSION//\'}
MAJOR_VERSION=${VERSION:5:1}
MINOR_VERSION=${VERSION:6:2}
BUG_FIX=${VERSION:8:2}

zip IPlugMultiTargets-v$MAJOR_VERSION.$MINOR_VERSION.$BUG_FIX-all.zip changelog.txt IPlugMultiTargets-mac.dmg IPlugMultiTargets-win-64bit.zip IPlugMultiTargets-win-32bit.zip