#! /bin/bash
echo "Starting Unix toolchain build of ardome-ml"
scons -j4 || exit 1
