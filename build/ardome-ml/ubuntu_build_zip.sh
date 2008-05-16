#!/bin/bash

cd tmp-build

echo "Compressing result ()"
tar -cjf $TARGET ardome-ml/
if [ ! $? == 0 ]; then
	echo Failed to compress
	exit 2
fi

echo "Copying result"
cp $TARGET ../

echo "removing temporary files"
rm -rf ardome-ml

cd ..
