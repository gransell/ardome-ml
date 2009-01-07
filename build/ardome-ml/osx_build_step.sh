#!/bin/bash

echo "Building Release and Debug versions..."
scons -j 8 full-install || exit 1

