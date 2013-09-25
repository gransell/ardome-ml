#!/bin/bash

if [ -z "$BAGTARGET" ]; then
	echo "Error: BAGTARGET must be set before running this script."
	exit 1
fi

#Only set for linux platforms. Not needed for win or osx.
if [[ "$BAGTARGET" =~ ^linux || "$BAGTARGET" =~ ^ubuntu || "$BAGTARGET" =~ ^gcov ]]; then
	export LD_LIBRARY_PATH=`pwd`/build/release/$BAGTARGET/lib/
	export AML_PATH=`pwd`/build/release/$BAGTARGET/lib/ardome-ml/openmedialib/plugins/
	export AMF_PREFIX_PATH=`pwd`/build/release/$BAGTARGET/
fi

build/release/$BAGTARGET/bin/opencore_unit_tests
ret1=$?
build/release/$BAGTARGET/bin/openmedialib_unit_tests
ret2=$?


if [ -n "$WORKSPACE" ]; then
  cp -v tests/test_output/opencore/opencore_test_results.xml $WORKSPACE/
  cp -v tests/test_output/openmedialib/unit_test_results.xml $WORKSPACE/
fi

if [ "$BAGTARGET" == "gcov" ]; then
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

