#!/bin/bash

echo "Trying to build openbuild.tbz2 target, creating directory named TMPZIP..."
rm -rf TMPZIP

mkdir TMPZIP || exit 1

echo "Adding cross-platform script files..."
cp -r openbuild TMPZIP/

if [ ! $? == 0 ]; then
	echo "Failed to copy script files files"
	exit 1
fi
echo "Purging .svn folders"
find TMPZIP -name \.svn -type d -prune -print0 | xargs -0 rm -rf

echo "Purging .pyc files"
find TMPZIP -name \*.pyc -type f -prune -print0 | xargs -0 rm -rf

echo "Building tar-file openbuild.tbz2 ..."
cd TMPZIP 
tar -jcf openbuild.tbz2 *

if [ ! $? == 0 ]; then
	echo "Failed to execture: tar -jcf openbuild.tar.bz2 *"
	exit 1
fi
	
cd ..	
echo "Copying openbuild.tbz2 to folder where prod was issued..."
mv TMPZIP/openbuild.tbz2 ./ 

echo "Removing temporary folder, almost done..."
rm -rf TMPZIP

echo "Product openbuild.tbz2 created successfully!"
