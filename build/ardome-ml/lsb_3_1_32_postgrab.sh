#!/bin/bash

cd bcomp

echo "Extracting loki..."
tar xjf loki.tbz2
if [ ! $? == 0 ]; then
  echo "Failed to unpack loki. Terminating."
  exit 1
fi

echo "Extracting boost..."
tar xjf boost.tbz2
if [ ! $? == 0 ]; then
  echo "Failed to unpack boost. Terminating."
  exit 1
fi

echo "Extracting ffmpeg..."
tar xjf ffmpeg.tbz2
if [ ! $? == 0 ]; then
  echo "Failed to unpack ffmpeg. Terminating."
  exit 1
fi

echo "Extracting xercesc..."
tar xjf xercesc.tbz2
if [ ! $? == 0 ]; then
  echo "Failed to unpack xercesc. Terminating."
  exit 1
fi

echo "Extracting SDL..."
tar xjf sdl.tbz2
if [ ! $? == 0 ]; then
  echo "Failed to unpack SDL. Terminating."
  exit 1
fi

echo "Extracting rubberband..."
tar xjf rubberband.tbz2
if [ ! $? == 0 ]; then
  echo "Failed to unpack rubberband. Terminating."
  exit 1
fi
