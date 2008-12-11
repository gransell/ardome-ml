#!/bin/bash
cd bcomp

if [ ! -d vs2008 ]; then
	echo "Creating vs2008 subdir to bcomp"
	mkdir vs2008
fi

cd vs2008

if [ ! -d boost ]; then
	echo "Extracting boost [~90 MB on disk]..."
	tar -jxf ../tmp/boost.tar.bz2
	if [ ! $? == 0 ]; then
		echo "Failed to run: tar -jxf ../tmp/boost.tar.bz2. Terminating."
		exit
	fi
fi

if [ ! -d xerces-c-src_2_8_0 ]; then

	echo "Extracting xerces for c++ [~56 MB]..."

	unzip -oq ../tmp/xerces.zip 

	if [ ! $? == 0 ]; then
	  echo "Failed to run: unzip -oq ../tmp/xerces.zip. Terminating."
	  exit 1
	fi
fi

cd ..

if [ ! -d common ] ; then
	mkdir common
fi

cd common

if [ ! -d ffmpeg/include ]; then
	echo "Extracting ffmpeg [~33 MB]..."
	tar -jxf ../tmp/ffmpeg.tbz2 
	
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

if [ ! -d  loki-0.1.6/include ]; then

	echo "Extracting Loki [~11 MB]..."
	unzip -oq ../tmp/loki-0.1.6.zip  

	if [ ! $? == 0 ]; then
	  echo "Failed to run unzip -oq ../tmp/loki-0.1.6.zip. Terminating."
	  exit
	fi
fi

echo "Postgrab succeeded!"
