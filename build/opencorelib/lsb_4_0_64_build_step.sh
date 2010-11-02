#! /bin/bash
echo "Starting Unix toolchain build of ardome-ml"
scons target=lsb_4_0_64 -j4 || exit 1
