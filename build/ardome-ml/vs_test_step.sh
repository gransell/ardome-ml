#!/bin/bash
if [ $# -ne 1 ]; then
    echo "Usage: vs_build_step.sh target (vs2008 or vs2010)"
    exit 1
fi

if [ $1 != "vs2008" -a $1 != "vs2010" ]; then
    echo "Unknown target: $1"
    exit 1
fi

TARGET=$1


echo -e "\n\n\n----- Starting test-suite for ardome-ml -----\n\n"

echo "Sleeping for 10 seconds to give the computer time to recover from the build..."
sleep 10

echo "Running unit tests for opencorelib ..."
build/release/$TARGET/bin/opencore_unit_tests.exe --log_level=all > core_unit_tests.log 2>&1 

if [ ! $? == 0 ]; then
  cat core_unit_tests.log
  echo "opencorelib's unit tests failed. Terminating."
  exit 1
fi

echo "Running unit tests for openmedialib ..."
build/release/$TARGET/bin/openmedialib_unit_tests.exe

if [ ! $? == 0 ]; then
  echo "openmedialib's unit tests failed. Terminating."
  exit 1
fi

if [ -n "$TEST_REPORT_PATH" ]; then
  cp tests/test_output/opencore/opencore_test_results.xml $TEST_REPORT_PATH/
  cp tests/test_output/openmedialib/unit_test_results.xml $TEST_REPORT_PATH/
fi

echo "All unit tests passed!"
echo -e "Testphase done. Continue with production...\n\n"
