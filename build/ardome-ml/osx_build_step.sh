#!/bin/bash

echo "Building Release and Debug versions..."
scons -j 8 wrappers=yes xcode_ver=4.2 install_name=@executable_path/../lib/ || exit 1

