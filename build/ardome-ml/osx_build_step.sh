#!/bin/bash

echo "Building Release and Debug versions..."
scons -j 2 || exit 1

