#!/bin/bash

echo -e "\n\n\n----- Starting test-suite for ardome-ml -----\n\n"

echo "Sleeping for 10 seconds to give the computer time to recover from the build..."
sleep 10

echo "Running unit tests for opencorelib ..."
build/release/vs2008/bin/opencore_unit_tests.exe all_tests --log_level=all > core_unit_tests.log 2>&1 

if [ ! $? == 0 ]; then
  cat core_unit_tests.log
  echo "opencorelib's unit tests failed. Terminating."
  exit 1
fi

echo "Running unit tests for openmedialib ..."
build/release/vs2008/bin/openmedialib_unit_tests.exe

if [ ! $? == 0 ]; then
  echo "openmedialib's unit tests failed. Terminating."
  exit 1
fi

echo "All unit tests passed!"
echo -e "Testphase done. Continue with production...\n\n"
