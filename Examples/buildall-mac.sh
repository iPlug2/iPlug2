#! /bin/sh

#bash shell script to build all the plugin projects in this directory for OSX. 
#you may need to modify this if you don't have the AAX SDK, or only want to build vst2 etc
#since the build will cancel if there are any errors

BASEDIR=$(dirname $0)

#here you can choose a target to build
# TARGET="All"
TARGET="APP"
#TARGET="VST2"
#TARGET="VST3"
#TARGET="AU"
#TARGET="AAX"

cd $BASEDIR

echo "building all example plugins..."

if [ -f build_errors.log ]
then
  rm build_errors.log
fi

for file in * 
do
  if [ "$file" == "IPlugReaperExtension" ]; then
    continue
  fi

  if [ "$file" == "IPlugResponsiveUI" ]; then
    continue
  fi

  if [ -d "$file/projects/$file-macOS.xcodeproj" ]
  then
    echo "building $file/projects/$file-macOS.xcodeproj $TARGET target"
    xcodebuild -project "$file/projects/$file-macOS.xcodeproj" -target $TARGET -configuration Release 2> ./build_errors.log

    if [ -s build_errors.log ]
    then
      echo "build failed due to following errors in $file"
      echo ""
      cat build_errors.log
      exit 1
    else
      if [ -f build_errors.log ]
      then
        rm build_errors.log
      fi
    fi

  fi
done


echo "done"
