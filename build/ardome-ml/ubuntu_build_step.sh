#! /bin/bash
echo "Starting Unix toolchain build of ardome-ml"
BCOMP=$PWD/bcomp

echo "Preparing configure ..."
#autoreconf -i
./bootstrap
if [ ! $? == 0 ]; then
    echo "autoreconf failed.  Terminating."
    exit 1
fi

echo "Running configure ...."
CPPFLAGS=-DOLIB_USE_UTF8 ./configure --prefix=$PWD/tmp-build/ardome-ml --disable-caca PKG_CONFIG_PATH=$BCOMP/ffmpeg/lib/pkgconfig
if [ ! $? == 0 ]; then
    echo "configure failed.  Terminating."
    exit 1
fi

echo "Building ..."
make
if [ ! $? == 0 ]; then
    echo "Build failed. Terminating."
    exit 1
fi

echo "Installing to temporary ..."
make install
if [ ! $? == 0 ]; then
    echo "Install failed. Terminating."
    exit 1
fi

echo "Building succeeded!"
