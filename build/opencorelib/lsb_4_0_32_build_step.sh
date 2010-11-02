#! /bin/bash
echo "Starting Unix toolchain build of ardome-ml"
scons target=lsb_4_0_32 -j4 || exit 1
