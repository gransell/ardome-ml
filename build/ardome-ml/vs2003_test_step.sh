#!/bin/bash

echo -e "\n\n\n----- Starting test-suite for ardome-ml -----\n\n"

echo "Running unit tests for opencorelib ..."
build/release/vs2003/bin/opencore_unit_tests.exe --log_level=all > core_unit_tests.log 2>&1 

if [ ! $? == 0 ]; then
  cat core_unit_tests.log
  echo "opencorelib's unit tests failed. Terminating."
  exit 1
fi

echo "All unit tests passed!"
echo -e "Testphase done. Continue with production...\n\n"
