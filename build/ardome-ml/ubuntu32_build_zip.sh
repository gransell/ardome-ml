#!/bin/bash

echo "Moving content from debug"
mkdir -p aml/debug || exit 1
mv build/debug/ubuntu32/bin build/debug/ubuntu32/include build/debug/ubuntu32/lib build/debug/ubuntu32/share aml/debug || exit 1

echo "Moving content from release"
mkdir -p aml/release || exit 1
mv build/release/ubuntu32/bin build/release/ubuntu32/include build/release/ubuntu32/lib build/release/ubuntu32/share aml/release || exit 1

echo "Compressing result ()"
tar -cjf $TARGET aml
if [ ! $? == 0 ]; then
	echo Failed to compress
	exit 2
fi

