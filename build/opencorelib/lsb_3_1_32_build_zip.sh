#!/bin/bash

echo "Moving content from debug"
mkdir -p aml/debug || exit 1
mv build/debug/lsb_3_1_32/bin build/debug/lsb_3_1_32/include build/debug/lsb_3_1_32/lib build/debug/lsb_3_1_32/share aml/debug || exit 1

echo "Moving content from release"
mkdir -p aml/release || exit 1
mv build/release/lsb_3_1_32/bin build/release/lsb_3_1_32/include build/release/lsb_3_1_32/lib build/release/lsb_3_1_32/share aml/release || exit 1

echo "Copying AML source"
cp -r src aml || exit 1

echo "Compressing result"
tar -cjf $TARGET aml
if [ ! $? == 0 ]; then
	echo Failed to compress
	exit 2
fi

