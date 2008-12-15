#!/bin/bash

echo "Building Release and Debug versions..."
scons -j 5 || exit 1

