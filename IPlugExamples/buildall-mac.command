#! /bin/sh

#bash shell script to build all the plugin projects in this directory for OSX. 
#you may need to modify this if you don't have the RTAS SDK, or only want to build vst2 etc
#since the build will cancel if there are any errors

BASEDIR=$(dirname $0)

cd $BASEDIR

echo "building all example plugins..."

for file in * 
do
#echo $file
	if [ -d "$file/$file.xcodeproj" ]
	then
		echo "building $file/$file.xcodeproj All targets"
		xcodebuild -project "$file/$file.xcodeproj" -target "All" -configuration Release 2> ./build_errors.log
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

    if [ -s build_errors.log ]
    then
      echo "build failed due to following errors in $file"
      echo ""
      cat build_errors.log
      exit 1
    else
     rm  build_errors.log
    fi

	fi
done


echo "done"
