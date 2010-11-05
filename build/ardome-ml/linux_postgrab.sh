#!/bin/bash

cd bcomp

for pkg in 'loki.tbz2' 'boost.tbz2' 'ffmpeg.tbz2' 'xercesc.tbz2' 'sdl.tbz2' 'rubberband.tbz2'; do
    if [ -f $pkg ]; then
        name=${pkg/\.tbz2/};
        echo "Extracting ${name}..."
        tar xfj $pkg
        if [ ! $? == 0 ]; then
            echo "Failed to unpack ${name}. Terminating."
            exit 1
        fi
    fi
done
