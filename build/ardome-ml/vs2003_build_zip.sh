#!/bin/bash

echo "Trying to build ardome-ml-vs2003.zip target, creating directory named TMPZIP..."
rm -rf TMPZIP

mkdir TMPZIP || exit 1
mkdir TMPZIP/release || exit 1
mkdir TMPZIP/debug || exit 1
mkdir TMPZIP/src || exit 1

echo "Adding release files..."
cp -r build/release/vs2003/bin TMPZIP/release/
cp -r build/release/vs2003/include TMPZIP/release/
cp -r build/release/vs2003/lib TMPZIP/release/

if [ ! $? == 0 ]; then
	echo "Failed to copy release files"
	exit 1
fi

	  
echo "Adding debug files..."
cp -r build/debug/vs2003/bin TMPZIP/debug/
cp -r build/debug/vs2003/include TMPZIP/debug/
cp -r build/debug/vs2003/lib TMPZIP/debug/


if [ ! $? == 0 ]; then
	echo "Failed to copy debug files"
	exit 1
fi

					
echo "Adding header files to be able to build, adding source files to be able to debug ..."	  
cp -r src/ TMPZIP/src/
if [ ! $? == 0 ]; then
	echo "Failed to copy include and source files"
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
