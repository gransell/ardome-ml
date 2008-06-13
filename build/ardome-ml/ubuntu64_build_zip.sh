#!/bin/bash

mkdir -p aml/debug
mv build/debug/ubuntu64/bin build/debug/ubuntu64/include build/debug/ubuntu64/lib64 build/debug/ubuntu64/share aml/debug

mkdir -p aml/release
mv build/release/ubuntu64/bin build/release/ubuntu64/include build/release/ubuntu64/lib64 build/release/ubuntu64/share aml/release

echo "Compressing result ()"
tar -cjf $TARGET aml
if [ ! $? == 0 ]; then
	echo Failed to compress
	exit 2
fi

