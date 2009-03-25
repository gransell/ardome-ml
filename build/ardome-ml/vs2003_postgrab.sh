#!/bin/bash
cd bcomp

if [ ! -d vs2003 ]; then
	echo "Creating vs2003 subdir to bcomp"
	mkdir vs2003
fi

cd vs2003

if [ ! -d boost_1_37_0 ]; then

	echo "Extracting boost [~90 MB when extracted]..."
	tar -jxf ../tmp/vc71-boost_1_37-wchar_t_on-python25.tar.bz2
	
	if [ ! $? == 0 ]; then
		echo "Failed to run: tar -jxf ../../tmp/vc71-boost_1_37-wchar_t_on-python25.tar.bz2. Terminating."
		exit
	fi
	
fi

if [ ! -d xerces-c-2.8.0 ]; then

	echo "Extracting xerces for c++ [~21 MB]..."

	tar -xjf ../tmp/vc71-xerces-c-2.8.0-wchar_t-on.tar.bz2

	if [ ! $? == 0 ]; then
	  echo "Failed to run: tar -xjf ../tmp/vc71-xerces-c-2.8.0-wchar_t-on.tar.bz2. Terminating."
	  exit 1
	fi
fi

cd ..

if [ ! -d  ffmpeg/include ]; then
	echo "Extracting ffmpeg [~33 MB]..."
	tar -zxf ../tmp/ffmpeg.tgz
	
	if [ ! $? == 0 ]; then
	  echo "Failed to run tar -zxf ../tmp/ffmpeg-win32-20080825.tgz. Terminating."
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
