#!/bin/bash

echo "Building debug version..."
scons -j 2 debug=1 || exit 1

echo "Building release version..."
scons -j 2 || exit 1

