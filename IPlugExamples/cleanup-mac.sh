#! /bin/sh

BASEDIR=$(dirname $0)

cd $BASEDIR

echo "removing all build-mac folders..."

for file in * 
do
#echo $file
	if [ -d "$file/build-mac" ]
	then
		echo "removing $file/build-mac"
		rm -f -r $file/build-mac
	fi
done

echo "done"
