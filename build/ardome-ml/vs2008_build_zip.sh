#!/bin/bash

echo "Trying to build ardome-ml-vs2008.zip target, creating directory named TMPZIP..."
rm -rf TMPZIP

mkdir TMPZIP || exit 1
mkdir TMPZIP/aml || exit 1
mkdir TMPZIP/aml/include || exit 1
mkdir TMPZIP/aml/include/ardome-ml || exit 1
mkdir TMPZIP/aml/bin || exit 1
mkdir TMPZIP/aml/lib || exit 1
mkdir TMPZIP/aml/bin/release || exit 1
mkdir TMPZIP/aml/bin/debug || exit 1
mkdir TMPZIP/aml/lib/release || exit 1
mkdir TMPZIP/aml/lib/debug || exit 1
mkdir TMPZIP/aml/lib/winconfig || exit 1
mkdir TMPZIP/aml/asts || exit 1
mkdir TMPZIP/aml/py || exit 1

echo "Adding release files..."
cp -r build/release/vs2008/bin/* TMPZIP/aml/bin/release/
cp -r build/release/vs2008/lib/* TMPZIP/aml/lib/release/

if [ ! $? == 0 ]; then
	echo "Failed to copy release files"
	exit 1
fi
	  
echo "Adding debug files..."
cp -r build/debug/vs2008/bin/* TMPZIP/aml/bin/debug/
cp -r build/debug/vs2008/lib/* TMPZIP/aml/lib/debug/

if [ ! $? == 0 ]; then
	echo "Failed to copy debug files"
	exit 1
fi

echo "Adding ASTs and python files for wrappers..."
cp -r build/debug/vs2008/asts/* TMPZIP/aml/asts/
cp -r build/debug/vs2008/py/* TMPZIP/aml/py/

echo "Adding include files"
cp -r src/opencorelib  TMPZIP/aml/include/ardome-ml/
cp -r src/openimagelib  TMPZIP/aml/include/ardome-ml/
cp -r src/openmedialib  TMPZIP/aml/include/ardome-ml/
cp -r src/openpluginlib  TMPZIP/aml/include/ardome-ml/

echo "Adding winconfig file"
cp  config/windows/ardome_ml.wc TMPZIP/aml/lib/winconfig/ 

echo "Adding openbuild"
cp -r openbuild TMPZIP/

echo "Purging .svn folders"
find TMPZIP -name \.svn -type d -prune -print0 | xargs -0 rm -rf

echo "Purging unit_tests files"
find TMPZIP/aml/bin/debug/ -name "*unit_tests.pdb" -type f -print0 | xargs -0 rm -f
find TMPZIP/aml/bin/release/ -name "*unit_tests.pdb" -type f -print0 | xargs -0 rm -f
find TMPZIP/aml/bin/debug/ -name "*unit_tests.exe" -type f -print0 | xargs -0 rm -f
find TMPZIP/aml/bin/debug/ -name "*Unittests*" -type f -print0 | xargs -0 rm -f

echo "Purging lib-files from the bin directory"
find TMPZIP/aml/bin/debug/ -name "*.lib" -type f -print0 | xargs -0 rm -f
find TMPZIP/aml/bin/release/ -name "*.lib" -type f -print0 | xargs -0 rm -f

echo "Purging .pyc-files "
find TMPZIP/openbuild/ -name "*.pyc" -type f -print0 | xargs -0 rm -f
find TMPZIP/aml/py  -name "*.pyc" -type f -print0 | xargs -0 rm -f

rm -rf TMPZIP/aml/bin/debug/plugins
rm -rf TMPZIP/aml/bin/debug/examples
rm -rf TMPZIP/aml/bin/release/plugins
rm -rf TMPZIP/aml/bin/release/examples

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
