#! /bin/bash
echo "Starting Unix toolchain build of ardome-ml"
scons target=linux32 -j4 || exit 1
