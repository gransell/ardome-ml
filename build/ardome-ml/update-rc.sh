#!/bin/bash

if [ "$BUILDNUMBER" != "LATEST" ]; then

echo "Updating RC files..."

pretag-rc src/opencorelib/cl/msw/core.rc
pretag-rc src/openimagelib/il/msw/imagelib.rc
pretag-rc src/openmedialib/ml/msw/medialib.rc
pretag-rc src/openmedialib/plugins/ardendo/aml_filters.rc
pretag-rc src/openmedialib/plugins/avformat/msw/avformatplugin.rc
pretag-rc src/openmedialib/plugins/decode/msw/decodeplugin.rc
pretag-rc src/openmedialib/plugins/gensys/msw/gensysplugin.rc
pretag-rc src/openmedialib/plugins/mlt/msw/mltplugin.rc
pretag-rc src/openmedialib/plugins/rubberband/msw/rubberbandplugin.rc
pretag-rc src/openmedialib/plugins/sdl/msw/sdlplugin.rc
pretag-rc src/openmedialib/plugins/sox/msw/soxplugin.rc
pretag-rc src/openmedialib/plugins/wav/msw/wavplugin.rc
pretag-rc src/openmedialib/tools/amlbatch/amlbatch.rc
pretag-rc src/openpluginlib/pl/msw/pluginlib.rc


echo "Updating RC files done."

fi 
