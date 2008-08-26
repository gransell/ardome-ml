#!/bin/bash
cd bcomp

if [ ! -d vs2008 ]; then
	echo "Creating vs2008 subdir to bcomp"
	mkdir vs2008
fi

cd vs2008

if [ ! -d boost-1_34_1 ]; then

	echo "Extracting boost [~150 MB]..."
	tar -zxf ../vs2008_boost/boost-1.34.1.tar.gz  

	if [ ! $? == 0 ]; then
		echo "Failed to run: tar -zxf ../vs2008_boost/boost-1.34.1.tar.gz. Terminating."
		exit
	fi
fi

if [ ! -d xerces-c-2_8_0 ]; then

	echo "Extracting xerces for c++ [~56 MB]..."

	tar -zxf ../vs2008_xerces/xerces-c-2_8_0.tar.gz 

	if [ ! $? == 0 ]; then
	  echo "Failed to run: tar -zxf ../vs2008_xerces/xerces-c-2_8_0.tar.gz. Terminating."
	  exit 1
	fi
fi

cd ..

if [ ! -d common ] ; then
	mkdir common
fi

cd common

if [ ! -d  ffmpeg/include ]; then
	echo "Extracting ffmpeg [~33 MB]..."
	tar -jxf ../tmp/ffmpeg-win32-20080825.tbz2 
	
	if [ ! $? == 0 ]; then
	  echo "Failed to run tar -jxf ../tmp/ffmpeg-win32-20080825.tbz2. Terminating."
	  exit
	fi
fi

cd ../..

if [ ! -d  bcomp/sdl/include ]; then

	echo "Extracting sdl [~1 MB]..."
	unzip -oq bcomp/sdl/sdl-win32.zip -d bcomp/ 

	if [ ! $? == 0 ]; then
	  echo "Failed to unzip bcomp/sdl/sdl-win32.zip. Terminating."
	  exit
	fi
fi

if [ ! -d  bcomp/loki-0.1.6/include ]; then

	echo "Extracting Loki [~11 MB]..."
	unzip -oq bcomp/loki/loki-0.1.6.zip -d bcomp/ 

	if [ ! $? == 0 ]; then
	  echo "Failed to unzip bcomp/loki/loki-0.1.6.zip. Terminating."
	  exit
	fi
fi

echo "Postgrab succeeded!"
