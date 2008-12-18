#!/bin/bash

echo "Trying to build ardome-ml-vs2008.zip target, creating directory named TMPZIP..."
rm -rf TMPZIP

mkdir TMPZIP || exit 1
mkdir TMPZIP/aml || exit 1
mkdir TMPZIP/aml/release || exit 1
mkdir TMPZIP/aml/debug || exit 1

echo "Adding release files..."
cp -r build/release/vs2008/bin TMPZIP/aml/release/
cp -r build/release/vs2008/include TMPZIP/aml/release/
cp -r build/release/vs2008/lib TMPZIP/aml/release/

if [ ! $? == 0 ]; then
	echo "Failed to copy release files"
	exit 1
fi

	  
echo "Adding debug files..."
cp -r build/debug/vs2008/bin TMPZIP/aml/debug/
cp -r build/debug/vs2008/include TMPZIP/aml/debug/
cp -r build/debug/vs2008/lib TMPZIP/aml/debug/


if [ ! $? == 0 ]; then
	echo "Failed to copy debug files"
	exit 1
fi

					
echo "Adding source files to be able to debug ..."	  
cp -r src/ TMPZIP/aml/
if [ ! $? == 0 ]; then
	echo "Failed to copy source files"
	exit 1
fi

echo "Purging .svn folders"
find TMPZIP -name \.svn -type d -prune -print0 | xargs -0 rm -rf

echo "Building zip-file ardome-ml-vs2008.zip .."
cd TMPZIP 
zip -mrq ardome-ml-vs2008.zip * 

if [ ! $? == 0 ]; then
	echo "Failed to execture: zip -mrq ardome-ml-vs2008.zip * "
	exit 1
fi
	
cd ..	
echo "Copying ardome-ml-vs2008.zip to folder where prod was issued..."
mv TMPZIP/ardome-ml-vs2008.zip ./ 

echo "Removing temporary folder, almost done..."
rm -rf TMPZIP

echo "Product ardome-ml-vs2008.zip created successfully!"
