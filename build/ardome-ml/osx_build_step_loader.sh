#!/bin/bash

echo "Building Release and Debug versions..."
scons -j 4 wrappers=no install_name=@loader_path/../lib/ || exit 1

