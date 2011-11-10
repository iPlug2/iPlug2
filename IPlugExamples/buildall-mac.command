#! /bin/sh

BASEDIR=$(dirname $0)

cd $BASEDIR

echo "building all example plugins..."

for file in * 
do
#echo $file
	if [ -d "$file/$file.xcodeproj" ]
	then
		echo "building $file/$file.xcodeproj All targets"
		xcodebuild -project "$file/$file.xcodeproj" -target "All" -configuration Release
# 		echo "building $file/$file.xcodeproj VST"
# 		xcodebuild -project "$file/$file.xcodeproj" -target "VST_32&64_intel" -configuration Release
# 		echo "building $file/$file.xcodeproj AU"
# 		xcodebuild -project "$file/$file.xcodeproj" -target "AU_32&64_intel" -configuration Release
# 		echo "building $file/$file.xcodeproj App"
# 		xcodebuild -project "$file/$file.xcodeproj" -target "OSXAPP_32&64_intel" -configuration Release
# 		echo "building $file/$file.xcodeproj VST3"
# 		xcodebuild -project "$file/$file.xcodeproj" -target "VST3_32&64_intel" -configuration Release
# 		echo "building $file/$file.xcodeproj RTAS"
# 		xcodebuild -project "$file/$file.xcodeproj" -target "RTAS_32_intel" -configuration Release
	fi
done

echo "done"
