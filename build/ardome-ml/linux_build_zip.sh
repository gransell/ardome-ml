#!/bin/bash

echo "Moving content from debug"
mkdir -p aml/debug || exit 1
mv build/debug/$BAGTARGET/bin build/debug/$BAGTARGET/include build/debug/$BAGTARGET/lib build/debug/$BAGTARGET/share aml/debug || exit 1

echo "Moving content from release"
mkdir -p aml/release || exit 1
mv build/release/$BAGTARGET/bin build/release/$BAGTARGET/include build/release/$BAGTARGET/lib build/release/$BAGTARGET/share aml/release || exit 1

echo "Moving license information"
mkdir -p aml/license || exit 1
mv build/release/$BAGTARGET/license aml/license

echo "Copying AML source"
cp -r src aml || exit 1

echo "Compressing result"
tar -cjf $TARGET aml
if [ ! $? == 0 ]; then
	echo Failed to compress
	exit 2
fi

