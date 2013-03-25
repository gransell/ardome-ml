#!/bin/bash
cd bcomp

if [ ! -d vs2010 ]; then
	echo "Creating vs2010 subdir to bcomp"
	mkdir vs2010
fi

cd vs2010

if [ ! -d boost_1_47_0 ]; then
	echo "Extracting boost [~140 MB on disk]..."
	unzip -oq ../tmp/vc100-boost_1_47_0.zip
	if [ ! $? == 0 ]; then
		echo "Failed to run: unzip -oq ../tmp/vc100-boost_1_47_0.zip. Terminating."
		exit
	fi
fi

if [ ! -d rubberband-1.5.0 ]; then
    echo "Extracting rubberband..."
    tar zxf ../tmp/rubberband-1.7.0-win32.tgz
    if [ ! $? == 0 ]; then
        echo "Failed to run: tar zxf rubberband-1.7.0-win32.tgz. Terminating."
        exit
    fi
fi

if [ ! -d openal ]; then
	echo "Extracting OpenAL..."
	unzip -oq ../tmp/openal.zip	
	if [ ! $? == 0 ]; then
		echo "Failed to run: unzip openal.zip. Terminating."
		exit
	fi
fi

if [ ! -d xerces-c-src_2_8_0 ]; then

	echo "Extracting xerces for c++ [~56 MB]..."

	unzip -oq ../tmp/vc100-xerces-c-2_8_0.zip 

	if [ ! $? == 0 ]; then
	  echo "Failed to run: unzip -oq ../tmp/vc100-xerces-c-2_8_0.zip. Terminating."
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

if [ ! -d directx  ]; then

    echo "Extracting DirectX [~27 MB]..."

    unzip -oq ../tmp/directx.zip

    if [ ! $? == 0 ]; then
        echo "Failed to unzip -oq ../tmp/directx.zip. Terminating."
        exit 1
    fi
fi

if [ ! -d openal  ]; then

    echo "Extracting OpenAL..."

    unzip -oq ../tmp/openal.zip

    if [ ! $? == 0 ]; then
        echo "Failed to unzip -oq ../tmp/openal.zip. Terminating."
        exit 1
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

if [ ! -d cairo_1.10.2-1_win32/include ]; then

	echo "Extracting cairo [~5 MB]..."
	unzip -oq ../tmp/cairo.zip  

	if [ ! $? == 0 ]; then
	  echo "Failed to run unzip -oq ../tmp/cairo.zip. Terminating."
	  exit
	fi
fi

if [ ! -d rsvg_2.32.1-1_win32/include ]; then

	echo "Extracting librsvg [~38 MB]..."
	unzip -oq ../tmp/rsvg.zip  

	if [ ! $? == 0 ]; then
	  echo "Failed to run unzip -oq ../tmp/rsvg.zip. Terminating."
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
	unzip -oq tmp/owl-vs2010.zip

	if [ ! $? == 0 ]; then
          echo "Failed to run unzip unzip -oq tmp/owl-vs2010.zip. Terminating."
          exit
        fi
fi

echo "Postgrab succeeded!"
