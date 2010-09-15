#!/bin/bash

echo "Building Release and Debug versions..."
scons -j 8 wrappers=yes install_name=@executable_path/../lib/ || exit 1

