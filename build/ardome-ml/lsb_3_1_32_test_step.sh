#!/bin/bash

export LD_LIBRARY_PATH=`pwd`/build/release/lsb_3_1_32/lib/
export AML_PATH=`pwd`/build/release/lsb_3_1_32/lib/ardome-ml/openmedialib/plugins/
export AMF_PREFIX_PATH=`pwd`/build/release/lsb_3_1_32/

build/release/lsb_3_1_32/bin/opencore_unit_tests all_tests

exit $?

