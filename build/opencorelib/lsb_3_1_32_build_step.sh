#! /bin/bash
echo "Starting Unix toolchain build of ardome-ml"
scons target=lsb_3_1_32 -j4 || exit 1
