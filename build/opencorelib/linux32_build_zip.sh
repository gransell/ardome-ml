#!/bin/bash

echo "Moving content from debug"
mkdir -p aml/debug || exit 1
mv build/debug/linux32/bin build/debug/linux32/include build/debug/linux32/lib build/debug/linux32/share aml/debug || exit 1

echo "Moving content from release"
mkdir -p aml/release || exit 1
mv build/release/linux32/bin build/release/linux32/include build/release/linux32/lib build/release/linux32/share aml/release || exit 1

echo "Copying AML source"
cp -r src aml || exit 1

echo "Compressing result"
tar -cjf $TARGET aml
if [ ! $? == 0 ]; then
	echo Failed to compress
	exit 2
fi

