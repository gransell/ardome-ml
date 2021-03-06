#!/bin/bash

echo "Copying libs and headers to temporary dir..."
mkdir -p aml/debug
mv build/debug/osx/bin build/debug/osx/include build/debug/osx/lib build/debug/osx/share aml/debug

mkdir -p aml/release
mv build/release/osx/bin build/release/osx/include build/release/osx/lib build/release/osx/share aml/release

echo Compressing result...
tar -cjf ardome-ml-osx-10.5.tbz2 aml/ > /dev/null
if [ ! $? == 0 ]; then
	echo Failed to compress
	exit 2
fi
