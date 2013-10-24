#!/bin/bash

function print_usage
{
	echo
	echo "Usage: test_step.sh [--xml] target"
	echo
	echo "Parameters:"
	echo "--xml:  If set, the test output will be in XML format instead of simple text." 
	echo "target: The current build target (same as for SCons)."
	exit 1
}

if [ $# -lt 1 -o $# -gt 2 ]; then
	print_usage
fi

XML_OUTPUT=0
if [ $# -eq 1 ]; then
	TARGET="$1"
else
	if [ "$1" == "--xml" ]; then
		XML_OUTPUT=1
		TARGET="$2"
	else
		print_usage
	fi
fi

#Catch attempt to interpret flag as TARGET
if [[ "$TARGET" =~ ^- ]]; then
	print_usage
fi

#Only set for linux platforms. Not needed for win or osx.
if [[ "$TARGET" =~ ^linux || "$TARGET" =~ ^ubuntu || "$TARGET" =~ ^gcov ]]; then
	export LD_LIBRARY_PATH=`pwd`/build/release/$TARGET/lib/
	export AML_PATH=`pwd`/build/release/$TARGET/lib/ardome-ml/openmedialib/plugins/
	export AMF_PREFIX_PATH=`pwd`/build/release/$TARGET/
fi

if [ $XML_OUTPUT -eq 1 ]; then
	TEST_FLAGS="--xml"
	OUTPUT_FILE_SUFFIX="xml"
else
	OUTPUT_FILE_SUFFIX="clf"
fi

build/release/$TARGET/bin/opencore_unit_tests "$TEST_FLAGS"
ret1=$?
build/release/$TARGET/bin/openmedialib_unit_tests "$TEST_FLAGS"
ret2=$?


if [ -n "$WORKSPACE" ]; then
	cp -v "tests/test_output/opencorelib/opencorelib_test_results.$OUTPUT_FILE_SUFFIX" $WORKSPACE/
	cp -v "tests/test_output/openmedialib/openmedialib_test_results.$OUTPUT_FILE_SUFFIX" $WORKSPACE/
fi

if [ "$TARGET" == "gcov" ]; then
	if [ -f "build/ardome-ml/gcovr.py" ]; then
		if [ -z "$WORKSPACE" ]; then
			WORKSPACE=`pwd`
		fi
		echo "Running gcov on all tests in $WORKSPACE"
		cp build/ardome-ml/gcovr.py .
		python gcovr.py --xml >> branch-coverage.xml
		cp branch-coverage.xml $WORKSPACE/tests/
		rm gcovr.py
	fi
fi

if [ $ret1 -ne 0 -o $ret2 -ne 0 ]; then
	exit 1
fi

exit 0

