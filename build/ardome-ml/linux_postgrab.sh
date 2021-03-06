#!/bin/bash

cd bcomp

for pkg in 'loki.tbz2' 'ffmpeg.tbz2' 'xercesc.tbz2' 'sdl.tbz2' 'rubberband.tbz2' 'cairo.tbz2' 'librsvg.tbz2' 'black_magic_decklink_sdk.tbz2' ; do
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

for pkg in 'boost.zip'; do
    if [ -f $pkg ]; then
        name=${pkg/\.zip/};
        echo "Extracting ${name}..."
        unzip -oq $pkg
        if [ ! $? == 0 ]; then
            echo "Failed to unpack ${name}. Terminating."
            exit 1
        fi
    fi
done

for pkg in 'cairo.tar.gz'; do
    if [ -f $pkg ]; then
        name=${pkg/\.tar.gz/};
        echo "Extracting ${name}..."
        tar xfz $pkg
        if [ ! $? == 0 ]; then
            echo "Failed to unpack ${name}. Terminating."
            exit 1
        fi
    fi
done

# Remove whitespace from directory name and normalise to avoid having to maintain version info in multiple places
mv 'Blackmagic DeckLink SDK 8.6' decklink_sdk
