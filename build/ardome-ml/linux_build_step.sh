#! /bin/bash
echo "Starting Unix toolchain build of ardome-ml"
scons target=$BAGTARGET -j`cat /proc/cpuinfo | grep processor | wc -l` || exit 1
