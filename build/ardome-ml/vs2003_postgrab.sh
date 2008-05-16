#!/bin/bash
cd bcomp

if [ ! -d vs2003 ]; then
	echo "Creating vs2003 subdir to bcomp"
	mkdir vs2003
fi

cd vs2003

if [ ! -d boost_1_34_1 ]; then

	echo "Extracting boost [~100 MB]..."
	tar -jxf ../vs2003_boost/vc71-boost_1_34_1.tar.bz2
	
	if [ ! $? == 0 ]; then
		echo "Failed to run: tar -jxf ../vs2003_boost/vc71-boost_1_34_1.tar.bz2. Terminating."
		exit
	fi
fi

if [ ! -d xerces-c_2_8_0-x86-windows-vc_7_1 ]; then

	echo "Extracting xerces for c++ [~21 MB]..."

	unzip -oq ../vs2003_xerces/vc71-xerces-c-2_8_0.zip 

	if [ ! $? == 0 ]; then
	  echo "Failed to run: unzip -oq ../vs2003_xerces/vc71-xerces-c-2_8_0.zip. Terminating."
	  exit 1
	fi
fi

cd ..
cd ..

if [ ! -d  bcomp/ffmpeg/include ]; then
	echo "Extracting ffmpeg [~33 MB]..."
	unzip -oq bcomp/ffmpeg/ffmpeg-win32.zip -d bcomp/ 

	if [ ! $? == 0 ]; then
	  echo "Failed to unzip bcomp/ffmpeg/ffmpeg-win32.zip. Terminating."
	  exit
	fi
fi

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
