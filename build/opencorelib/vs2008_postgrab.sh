#!/bin/bash
cd bcomp

if [ ! -d vs2008 ]; then
	echo "Creating vs2008 subdir to bcomp"
	mkdir vs2008
fi

if [ ! -d common ]; then
	echo "Creating common subdir to bcomp"
	mkdir common
fi

cd vs2008

if [ ! -d boost ]; then

	echo "Extracting boost [~150 MB]..."
	tar -jxf ../tmp/boost.tar.bz2
	if [ ! $? == 0 ]; then
		echo "Failed to run: tar -jxf ../tmp/boost.tar.bz2. Terminating."
		exit
	fi
fi

if [ ! -d xerces-c-3.0.0-x86-windows-vc-9.0 ]; then

	echo "Extracting xerces for c++ [~56 MB]..."

	unzip -oq ../tmp/xerces.zip 

	if [ ! $? == 0 ]; then
	  echo "Failed to run: unzip -oq ../tmp/xerces.zip. Terminating."
	  exit 1
	fi
fi

cd ..
cd common

if [ ! -d ffmpeg/include ]; then
	echo "Extracting ffmpeg [~33 MB]..."
	tar -jxf ../tmp/ffmpeg.tbz2 
	
	if [ ! $? == 0 ]; then
	  echo "Failed to run tar -jxf ../tmp/ffmpeg-win32-20080825.tbz2. Terminating."
	  exit
	fi
fi

if [ ! -d sdl/include ]; then

	echo "Extracting sdl [~1 MB]..."
	tar -jxf ../tmp/sdl-win32.B.tar.bz2

	if [ ! $? == 0 ]; then
	  echo "Failed to run tar -jxf ../tmp/sdl-win32.B.tar.bz2. Terminating."
	  exit
	fi
fi

if [ ! -d loki-0.1.6/include ]; then

	echo "Extracting Loki [~11 MB]..."
	unzip -oq ../tmp/loki-0.1.6.zip

	if [ ! $? == 0 ]; then
	  echo "Failed to unzip ../tmp/loki-0.1.6.zip. Terminating."
	  exit
	fi
fi

echo "Postgrab succeeded!"
