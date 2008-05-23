#!/bin/bash

echo "Building Release version..."
scons -j 2 debug=0 || exit 1

echo "Building Debug version..."
scons -j 2 || exit 1

