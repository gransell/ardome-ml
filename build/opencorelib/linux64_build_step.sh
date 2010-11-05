#! /bin/bash
echo "Starting Unix toolchain build of ardome-ml"
scons target=linux64 -j4 || exit 1
