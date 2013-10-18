#!/bin/bash

echo "Copying libs and headers to temporary dir..."
mkdir -p aml/debug
mv build/debug/osx/bin build/debug/osx/include build/debug/osx/lib build/debug/osx/Resources build/debug/osx/share aml/debug

mkdir -p aml/release
mv build/release/osx/bin build/release/osx/include build/release/osx/lib build/release/osx/Resources build/release/osx/share aml/release

mv build/release/osx/license aml

echo Compressing result...
tar -cjf $1.tbz2 aml/ > /dev/null
if [ ! $? == 0 ]; then
	echo Failed to compress
	exit 2
fi
