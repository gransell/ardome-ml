#!/bin/bash

export LD_LIBRARY_PATH=`pwd`/build/release/$BAGTARGET/lib/
export AML_PATH=`pwd`/build/release/$BAGTARGET/lib/ardome-ml/openmedialib/plugins/
export AMF_PREFIX_PATH=`pwd`/build/release/$BAGTARGET/

build/release/$BAGTARGET/bin/opencore_unit_tests all_tests

exit $?

