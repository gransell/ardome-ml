#! /bin/bash
echo "Starting Unix toolchain build of ardome-ml"
scons target=lsb_3_1_64 -j4 || exit 1
