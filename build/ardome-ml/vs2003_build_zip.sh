#!/bin/bash

echo "Trying to build ardome-ml-vs2003.zip target, creating directory named TMPZIP..."
rm -rf TMPZIP

mkdir TMPZIP || exit 1
mkdir TMPZIP/bin/ || exit 1
mkdir TMPZIP/bin/debug || exit 1
mkdir TMPZIP/bin/debug/aml-plugins || exit 1
mkdir TMPZIP/bin/release || exit 1
mkdir TMPZIP/bin/release/aml-plugins || exit 1
mkdir TMPZIP/src || exit 1

echo "Adding release binaries..."
cp -r bcomp/ardome-ml/vs2003/bin_vs2003/release/*.dll \
      bcomp/ardome-ml/vs2003/bin_vs2003/release/*.lib \
	  bcomp/ardome-ml/vs2003/bin_vs2003/release/*.pdb \
      TMPZIP/bin/release 
	  
if [ ! $? == 0 ]; then
	echo "Failed to copy binary files"
	exit 1
fi

echo "Adding release plugins..."
cp -r bcomp/ardome-ml/vs2003/bin_vs2003/release/aml-plugins/*.dll \
	  bcomp/ardome-ml/vs2003/bin_vs2003/release/aml-plugins/*.pdb \
      TMPZIP/bin/release/aml-plugins 

if [ ! $? == 0 ]; then
	echo "Failed to copy binary files"
	exit 1
fi

echo "Adding opl-files..."
cp bcomp/ardome-ml/src/openmedialib/plugins/avformat/avformat_plugin.opl TMPZIP/bin/release/aml-plugins 
cp bcomp/ardome-ml/src/openmedialib/plugins/gensys/gensys_plugin.opl TMPZIP/bin/release/aml-plugins
cp bcomp/ardome-ml/src/openmedialib/plugins/sdl/sdl_plugin.opl TMPZIP/bin/release/aml-plugins
cp bcomp/ardome-ml/src/openimagelib/plugins/gdi+/gdi_plugin.opl TMPZIP/bin/release/aml-plugins

cp bcomp/ardome-ml/src/openmedialib/plugins/avformat/avformat_plugin.opl TMPZIP/bin/debug/aml-plugins 
cp bcomp/ardome-ml/src/openmedialib/plugins/gensys/gensys_plugin.opl TMPZIP/bin/debug/aml-plugins
cp bcomp/ardome-ml/src/openmedialib/plugins/sdl/sdl_plugin.opl TMPZIP/bin/debug/aml-plugins
cp bcomp/ardome-ml/src/openimagelib/plugins/gdi+/gdi_plugin.opl TMPZIP/bin/debug/aml-plugins

if [ ! $? == 0 ]; then
	echo "Failed to copy opl files"
	exit 1
fi

echo "Adding debug binaries..."	  
cp -r bcomp/ardome-ml/vs2003/bin_vs2003/debug/*.dll \
      bcomp/ardome-ml/vs2003/bin_vs2003/debug/*.lib \
	  bcomp/ardome-ml/vs2003/bin_vs2003/debug/*.pdb \
      TMPZIP/bin/debug

if [ ! $? == 0 ]; then
	echo "Failed to copy binary files"
	exit 1
fi

echo "Adding debug plugins..."	  
cp -r bcomp/ardome-ml/vs2003/bin_vs2003/debug/aml-plugins/*.dll \
	  bcomp/ardome-ml/vs2003/bin_vs2003/debug/aml-plugins/*.pdb \
      TMPZIP/bin/debug/aml-plugins 

if [ ! $? == 0 ]; then
	echo "Failed to copy binary files"
	exit 1
fi
					
echo "Adding header files to be able to build, adding source files to be able to debug ..."	  
cp -r bcomp/ardome-ml/src/* TMPZIP/src/
if [ ! $? == 0 ]; then
	echo "Failed to copy include and source files"
	exit 1
fi

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
