#!/bin/bash
cd bcomp

if [ ! -d vs2008 ]; then
	echo "Creating vs2008 subdir to bcomp"
	mkdir vs2008
fi

cd vs2008

if [ ! -d boost_1_37_0 ]; then
	echo "Extracting boost [~140 MB on disk]..."
	tar -jxf ../tmp/vc90-boost_1_37-wchar_t-on.tar.bz2
	if [ ! $? == 0 ]; then
		echo "Failed to run: tar -jxf ../tmp/vc90-boost_1_37-wchar_t-on.tar.bz2. Terminating."
		exit
	fi
fi

if [ ! -d rubberband ]; then
	echo "Extracting rubberband..."
	unzip ../tmp/rubberband-1.5.0.zip	
	if [ ! $? == 0 ]; then
		echo "Failed to run: unzip rubberband.zip. Terminating."
		exit
	fi
fi

if [ ! -d xerces-c-src_2_8_0 ]; then

	echo "Extracting xerces for c++ [~56 MB]..."

	unzip -oq ../tmp/vc90-xerces-c-2_8_0.zip 

	if [ ! $? == 0 ]; then
	  echo "Failed to run: unzip -oq ../tmp/vc90-xerces-c-2_8_0.zip. Terminating."
	  exit 1
	fi
fi

if [ ! -d ffmpeg/include ]; then
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

echo "Extracting Doxygen..."
unzip -oq ../tmp/doxygen.zip
if [ ! $? == 0 ]; then
  echo "Failed to run unzip -oq ../tmp/doxygen.zip. Terminating."
  exit
fi

cd ..

if [ ! -d owl ]; then
	
	echo "Extracting OWL [~3 MB]..."
	unzip -oq tmp/owl-vs2008.zip

	if [ ! $? == 0 ]; then
          echo "Failed to run unzip unzip -oq tmp/owl-vs2008.zip. Terminating."
          exit
        fi
fi

echo "Postgrab succeeded!"
