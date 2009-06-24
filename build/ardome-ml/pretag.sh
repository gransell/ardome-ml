#!/bin/bash

str="$PRODUCT $MAJORVER.$MINORVER (build $BUILDNUMBER)"

echo $str > LAST_TAG

bzr ci LAST_TAG -m "Version number updated - $str"
