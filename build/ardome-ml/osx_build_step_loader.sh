#!/bin/bash

echo "Building Release and Debug versions..."
scons -j 8 debug release full-install || exit 1

