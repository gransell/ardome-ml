#!/bin/bash

mkdir -p aml/debug
mv build/debug/ubuntu32/bin build/debug/ubuntu32/include build/debug/ubuntu32/lib build/debug/ubuntu32/share aml/debug

mkdir -p aml/release
mv build/release/ubuntu32/bin build/release/ubuntu32/include build/release/ubuntu32/lib build/release/ubuntu32/share aml/release

echo "Compressing result ()"
tar -cjf $TARGET aml
if [ ! $? == 0 ]; then
	echo Failed to compress
	exit 2
fi

