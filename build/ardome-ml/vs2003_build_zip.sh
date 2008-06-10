#!/bin/bash

echo "Trying to build ardome-ml-vs2003.zip target, creating directory named TMPZIP..."
rm -rf TMPZIP

mkdir TMPZIP || exit 1
mkdir TMPZIP/ardome-ml || exit 1
mkdir TMPZIP/ardome-ml/release || exit 1
mkdir TMPZIP/ardome-ml/debug || exit 1
mkdir TMPZIP/ardome-ml/src || exit 1

echo "Adding release files..."
cp -r build/release/vs2003/bin TMPZIP/ardome-ml/release/
cp -r build/release/vs2003/include TMPZIP/ardome-ml/release/
cp -r build/release/vs2003/lib TMPZIP/ardome-ml/release/

if [ ! $? == 0 ]; then
	echo "Failed to copy release files"
	exit 1
fi

	  
echo "Adding debug files..."
cp -r build/debug/vs2003/bin TMPZIP/ardome-ml/debug/
cp -r build/debug/vs2003/include TMPZIP/ardome-ml/debug/
cp -r build/debug/vs2003/lib TMPZIP/ardome-ml/debug/


if [ ! $? == 0 ]; then
	echo "Failed to copy debug files"
	exit 1
fi

					
echo "Adding source files to be able to debug ..."	  
cp -r src/ TMPZIP/ardome-ml/src/
if [ ! $? == 0 ]; then
	echo "Failed to copy source files"
	exit 1
fi

echo "Purging .svn folders"
find TMPZIP -name \.svn -type d -prune -print0 | xargs -0 rm -rf

echo "Building zip-file ardome-ml-vs2003.zip .."
cd TMPZIP 
zip -mrq ardome-ml-vs2003.zip * 

if [ ! $? == 0 ]; then
	echo "Failed to execture: zip -mrq ardome-ml-vs2003.zip * "
	exit 1
fi
	
cd ..	
echo "Copying ardome-ml-vs2003.zip to folder where prod was issued..."
mv TMPZIP/ardome-ml-vs2003.zip ./ 

echo "Removing temporary folder, almost done..."
rm -rf TMPZIP

echo "Product ardome-ml-vs2003.zip created successfully!"
