#!/bin/bash

echo "Moving content from debug"
mkdir -p aml/debug || exit 1
mv build/debug/ubuntu64/bin build/debug/ubuntu64/include build/debug/ubuntu64/lib64 build/debug/ubuntu64/share aml/debug || exit 1

echo "Moving content from release"
mkdir -p aml/release || exit 1
mv build/release/ubuntu64/bin build/release/ubuntu64/include build/release/ubuntu64/lib64 build/release/ubuntu64/share aml/release || exit 1

echo "Compressing result"
tar -cjf $TARGET aml
if [ ! $? == 0 ]; then
	echo Failed to compress
	exit 2
fi

