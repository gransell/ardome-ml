#!/bin/bash
cd bcomp

if [ ! -d vs2003 ]; then
	echo "Creating vs2003 subdir to bcomp"
	mkdir vs2003
fi

if [ ! -d common ]; then
	echo "Creating common subdir to bcomp"
	mkdir common
fi

cd vs2003

if [ ! -d boost_1_34_1 ]; then

	echo "Extracting boost [~100 MB]..."
	tar -jxf ../tmp/vc71-boost_1_34_1.B.tar.bz2
	
	if [ ! $? == 0 ]; then
		echo "Failed to run: tar -jxf ../tmp/vc71-boost_1_34_1.B.tar.bz2. Terminating."
		exit
	fi
fi

if [ ! -d xerces-c_2_8_0-x86-windows-vc_7_1 ]; then

	echo "Extracting xerces for c++ [~21 MB]..."

	unzip -oq ../tmp/vc71-xerces-c-2_8_0.zip 

	if [ ! $? == 0 ]; then
	  echo "Failed to run: unzip -oq ../tmp/vc71-xerces-c-2_8_0.zip. Terminating."
	  exit 1
	fi
fi

cd ..
cd common

if [ ! -d  ffmpeg/include ]; then
	echo "Extracting ffmpeg [~33 MB]..."
	tar -jxf ../tmp/ffmpeg.tbz2
	
	if [ ! $? == 0 ]; then
	  echo "Failed to run tar -jxf ../tmp/ffmpeg-win32-20080825.tbz2. Terminating."
	  exit
	fi
fi

if [ ! -d  sdl/include ]; then

	echo "Extracting sdl [~1 MB]..."
	tar -jxf ../tmp/sdl-win32.B.tar.bz2

	if [ ! $? == 0 ]; then
	  echo "Failed to run tar -jxf ../tmp/sdl-win32.B.tar.bz2. Terminating."
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
