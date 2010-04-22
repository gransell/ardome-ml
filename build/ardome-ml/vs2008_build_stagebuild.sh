#!/bin/bash

#This is quite generic, the only assumption is that
#the target for stagebuild ends with sb.zip and the original just .zip
#In the amf case this is amf-vs2008sb.zip and the original amf-vs2008.zip 
export MINOR=${MINORVER/\./_};
export FNAMEBASE=${TARGET/.zip/};
export FIXEDBAGTARGET=${BAGTARGET/sb/};
if [ -z "$BRANCHVER" ]; then
	export FILENAME=$FNAMEBASE-${MAJORVER}_${MINOR}-B$BUILDNUMBER.zip
else 
	export FILENAME=$FNAMEBASE-${MAJORVER}_${MINOR}-$BRANCHVER-B$BUILDNUMBER.zip
fi

mkdir TMP_STAGEBUILD
cd TMP_STAGEBUILD
if [ -f "$FILENAME" ]; then
	rm -f "$FILENAME"
fi
echo "Fetching http://releases.ardendo.se/builds/$PRODUCT/$FIXEDBAGTARGET/$FILENAME"
wget --quiet http://releases.ardendo.se/builds/$PRODUCT/$FIXEDBAGTARGET/$FILENAME

if [ ! -f "$FILENAME" ]; then
	echo "COuldn't download $FILENAME. Note, that it has to be baged before this can be built."
	exit -1
fi

echo "Unpacking $FILENAME in the temporary folder TMP_STAGEBUILD"
unzip $FILENAME
mkdir build

#Product specific repacking stuff
mv aml/bin/debug build/Debug
mv aml/bin/release build/Release
mv aml/include build/include
mv aml build/aml
mv openbuild build/openbuild

#Generic package building things
cp ../build/$PRODUCT/Package.info build/Package.info
cd build
zip -R ../../$TARGET "*"

if [ ! -f ../../$TARGET ]; then
	echo "Failed to create zip";
	exit -2
fi

cd ../..
rm -rf TMP_STAGEBUILD
rm -f "$FILENAME"

