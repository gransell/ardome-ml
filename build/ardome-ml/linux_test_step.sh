#!/bin/bash

set -e

export LD_LIBRARY_PATH=`pwd`/build/release/$BAGTARGET/lib/
export AML_PATH=`pwd`/build/release/$BAGTARGET/lib/ardome-ml/openmedialib/plugins/
export AMF_PREFIX_PATH=`pwd`/build/release/$BAGTARGET/

build/release/$BAGTARGET/bin/opencore_unit_tests
build/release/$BAGTARGET/bin/openmedialib_unit_tests

if [ -n "$TEST_REPORT_PATH" ]; then
  cp tests/test_output/opencore/opencore_test_results.xml $TEST_REPORT_PATH/
  cp tests/test_output/openmedialib/unit_test_results.xml $TEST_REPORT_PATH/
fi

if [ $BAGTARGET == "gcov" ]; then
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

