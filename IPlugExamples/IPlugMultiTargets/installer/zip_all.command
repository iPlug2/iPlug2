BASEDIR=$(dirname $0)

cd $BASEDIR

VERSION=`echo | grep PLUG_VER ../resource.h`
VERSION=${VERSION//\#define PLUG_VER }
VERSION=${VERSION//\'}
MAJOR_VERSION=$(($VERSION & 0xFFFF0000))
MAJOR_VERSION=$(($MAJOR_VERSION >> 16)) 
MINOR_VERSION=$(($VERSION & 0x0000FF00))
MINOR_VERSION=$(($MINOR_VERSION >> 8)) 
BUG_FIX=$(($VERSION & 0x000000FF))

zip IPlugMultiTargets-v$MAJOR_VERSION.$MINOR_VERSION.$BUG_FIX-all.zip changelog.txt IPlugMultiTargets-mac.dmg IPlugMultiTargets-win-64bit.zip IPlugMultiTargets-win-32bit.zip