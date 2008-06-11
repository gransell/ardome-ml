#!/bin/bash

echo "Copying libs and headers to temporary dir..."
mkdir -p tmp/ardome-ml/lib

cp -rf build/release/osx/lib tmp/ardome-ml/lib/release
cp -rf build/debug/osx/lib tmp/ardome-ml/lib/debug
cp -rf src/opencorelib/cl/schemas tmp/ardome-ml/
cp -rf build/release/osx/include tmp/ardome-ml/

cd tmp

echo Compressing result...
tar -cjf ardome-ml-osx-10.5.tbz2 ardome-ml/ > /dev/null
if [ ! $? == 0 ]; then
	echo Failed to compress
	exit 2
fi

echo Copying result
cp ardome-ml-osx-10.5.tbz2 ../

cd ..
rm -rf tmp

