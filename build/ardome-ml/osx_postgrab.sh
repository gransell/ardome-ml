#!/bin/bash

echo Entering bcomp directory
cd bcomp

echo Extracting sdl...
tar -jxf SDL.tar.bz2 || exit 1

echo Changing install names
find ./ -name "*.dylib" |
(
while read dylib ;
    do
    if [ ! -h $dylib ]
        then
        id=`otool -L $dylib | head -2 | tail -1 | sed 's/(.*$//'`
        deps=`otool -L $dylib | tail -n +3 | grep @loader_path | sed 's/(.*$//'`

        if [ "`echo $id | grep @loader_path`" != "" ]
            then
            name=`echo $id | sed 's?.*@loader_path/??'`
            install_name_tool -id @loader_path/../lib/$name $dylib
        fi

        for d in $deps
            do
            name=`echo $d | sed 's?.*@loader_path/??'`
            install_name_tool -change $d @loader_path/../lib/$name $dylib
        done
    fi
done
)

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
