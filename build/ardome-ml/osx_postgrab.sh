#!/bin/bash

echo Entering bcomp directory
cd bcomp

echo "Extracting boost..."
tar -jxf boost.tar.bz2 || exit 1

echo Extracting ffmpeg...
tar -jxf ffmpeg.tar.bz2 || exit 1

echo Extracting sdl...
tar -jxf SDL.tar.bz2 || exit 1

echo Extracting Xercesc...
tar -jxf xercesc.tar.bz2 || exit 1

echo Extracting loki...
tar -xjf loki.tbz2

