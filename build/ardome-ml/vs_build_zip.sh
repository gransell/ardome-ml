#!/bin/bash
if [ $# -ne 1 ]; then
    echo "Usage: vs_build_step.sh target (vs2008 or vs2010)"
    exit 1
fi

if [ $1 != "vs2008" -a $1 != "vs2010" ]; then
    echo "Unknown target: $1"
    exit 1
fi

TARGET=$1

echo "Trying to build ardome-ml-$TARGET.zip target, creating directory named TMPZIP..."
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
mkdir TMPZIP/aml/doc/ || exit 1
mkdir TMPZIP/aml/license || exit 1

echo "Adding release files..."
cp -r build/release/$TARGET/bin/* TMPZIP/aml/bin/release/
cp -r build/release/$TARGET/lib/* TMPZIP/aml/lib/release/

if [ ! $? == 0 ]; then
	echo "Failed to copy release files"
	exit 1
fi
	  
echo "Adding debug files..."
cp -r build/debug/$TARGET/bin/* TMPZIP/aml/bin/debug/
cp -r build/debug/$TARGET/lib/* TMPZIP/aml/lib/debug/

cp bcomp/$TARGET/ffmpeg/lib/*.lib TMPZIP/aml/lib/

if [ ! $? == 0 ]; then
	echo "Failed to copy debug files"
	exit 1
fi

echo "Adding ASTs and python files for wrappers..."
cp -r build/debug/$TARGET/asts/* TMPZIP/aml/asts/
cp -r build/debug/$TARGET/py/* TMPZIP/aml/py/

echo "Adding include files"
cp -r src/opencorelib  TMPZIP/aml/include/ardome-ml/
cp -r src/openimagelib  TMPZIP/aml/include/ardome-ml/
cp -r src/openmedialib  TMPZIP/aml/include/ardome-ml/
cp -r src/openpluginlib  TMPZIP/aml/include/ardome-ml/
mkdir TMPZIP/aml/include/ardome-ml/openmedialib/ml/unittest
cp -r tests/openmedialib/unit_tests/mocks TMPZIP/aml/include/ardome-ml/openmedialib/ml/unittest/

cp -r bcomp/$TARGET/ffmpeg/include/* TMPZIP/aml/include/

echo "Adding winconfig files"
cp  config/windows/ardome_ml.wc TMPZIP/aml/lib/winconfig/ 
cp -r bcomp/$TARGET/ffmpeg/lib/winconfig/*.wc TMPZIP/aml/lib/winconfig/

echo "Adding openbuild"
cp -r openbuild TMPZIP/

echo "Adding documentation: AMF.chm "
#mv doc/html/AML.chm TMPZIP/aml/doc/
mv doc/html TMPZIP/aml/doc/html

echo "Adding licensing information"
cp -r build/release/$TARGET/license/* TMPZIP/aml/license/

echo "Purging .svn folders"
find TMPZIP -name \.svn -type d -prune -print0 | xargs -0 rm -rf

echo "Purging unit_tests files"
find TMPZIP/aml/bin/debug/ -name "*unit_tests.pdb" -type f -print0 | xargs -0 rm -f
find TMPZIP/aml/bin/release/ -name "*unit_tests.pdb" -type f -print0 | xargs -0 rm -f
find TMPZIP/aml/bin/debug/ -name "*unit_tests.exe" -type f -print0 | xargs -0 rm -f
find TMPZIP/aml/bin/debug/ -name "*Unittests*" -type f -print0 | xargs -0 rm -f

echo "Purging lib-files from the bin directory"
find TMPZIP/aml/bin/debug/ -name "*.lib" -type f -print0 | xargs -0 rm -f
find TMPZIP/aml/bin/debug/plugins -name "*.lib" -type f -print0 | xargs -0 rm -f
find TMPZIP/aml/bin/release/ -name "*.lib" -type f -print0 | xargs -0 rm -f
find TMPZIP/aml/bin/release/plugins -name "*.lib" -type f -print0 | xargs -0 rm -f

echo "Purging .pyc-files "
find TMPZIP/openbuild/ -name "*.pyc" -type f -print0 | xargs -0 rm -f
find TMPZIP/aml/py  -name "*.pyc" -type f -print0 | xargs -0 rm -f

echo "Building zip-file ardome-ml-$TARGET.zip .."
cd TMPZIP 
zip -mrq ardome-ml-$TARGET.zip * 

if [ ! $? == 0 ]; then
	echo "Failed to execture: zip -mrq ardome-ml-$TARGET.zip * "
	exit 1
fi
	
cd ..	
echo "Copying ardome-ml-$TARGET.zip to folder where prod was issued..."
mv TMPZIP/ardome-ml-$TARGET.zip ./ 

echo "Removing temporary folder, almost done..."
rm -rf TMPZIP

echo "Product ardome-ml-$TARGET.zip created successfully!"
