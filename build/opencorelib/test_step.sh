#!/bin/bash

echo "Running ctest to verify produced code quality ..."
ctest -D Experimental -A prod_version.txt || ( echo "Tests phase failed, prodoction cancelled!" && exit 1 )

