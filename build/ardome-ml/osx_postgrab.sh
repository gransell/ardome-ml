#!/bin/bash

echo Entering bcomp directory
cd bcomp

if [ -e "SDL.tar.bz2" ] 
	then
	echo Extracting sdl...
	tar -jxf SDL.tar.bz2 || exit 1
fi

if [ -e "xercesc.tar.bz2" ] 
	then
	echo Extracting Xercesc...
	tar -jxf xercesc.tar.bz2 || exit 1
fi

if [ -e "boost.tar.bz2" ] 
	then
	echo "Extracting boost..."
	tar -jxf boost.tar.bz2 || exit 1
fi

if [ -e "ffmpeg.tar.bz2" ] 
	then
	echo Extracting ffmpeg...
	tar -jxf ffmpeg.tar.bz2 || exit 1
fi

if [ -e "loki.tbz2" ] 
	then
	echo Extracting loki...
	tar -xjf loki.tbz2 || exit 1
fi

if [ -e "doxygen.tbz2" ] 
	then
	echo 'Extracting doxygen'
	tar xjf doxygen.tbz2 || exit 1
fi

if [ -e "sox.tbz2" ] 
	then
	echo 'Extracing sox'
	tar xjf sox.tbz2 || exit 1
fi

if [ -e "rubberband.tbz2" ] 
	then
	echo 'Extracting Rubberband'
	tar xjf rubberband.tbz2 || exit 1
fi

if [ -e "black_magic_decklink_sdk.tbz2" ]
then
	echo 'Extracting decklink'
	tar xjf black_magic_decklink_sdk.tbz2 || exit 1
	mv 'Blackmagic DeckLink SDK 8.6' decklink_sdk
fi
