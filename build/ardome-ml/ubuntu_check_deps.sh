#!/bin/bash

missing=''
for pkg in 'libloki-dev' 'libxerces-c2-dev' 'uuid-dev' 'libsdl1.2-dev' 'libboost-dev' 'libboost-date-time-dev' 'libboost-filesystem-dev' 'libboost-python-dev' 'libboost-regex-dev' 'libboost-signals-dev' 'libboost-test-dev' 'libboost-thread-dev'
do
	if dpkg-query --status $pkg > /dev/null 2>&1; then
    	: ok
	else
		missing="$missing$pkg "
	fi
done

if [ "$missing" != "" ]
then	echo "Please install the following packages: $missing"
		exit 1
fi
