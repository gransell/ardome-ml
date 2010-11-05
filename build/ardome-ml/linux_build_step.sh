#! /bin/bash
echo "Starting Unix toolchain build of ardome-ml"
scons target=$BAGTARGET -j4 || exit 1
