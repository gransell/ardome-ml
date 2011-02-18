#!/bin/bash

if [ "$BUILDNUMBER" != "LATEST" ]; then

echo "Updating RC files..."

prebag-rc src/opencorelib/cl/msw/core.rc
prebag-rc src/openimagelib/il/msw/imagelib.rc
prebag-rc src/openmedialib/ml/msw/medialib.rc
prebag-rc src/openmedialib/plugins/ardendo/aml_filters.rc
prebag-rc src/openmedialib/plugins/avformat/msw/avformatplugin.rc
prebag-rc src/openmedialib/plugins/decode/msw/decodeplugin.rc
prebag-rc src/openmedialib/plugins/gensys/msw/gensysplugin.rc
prebag-rc src/openmedialib/plugins/mlt/msw/mltplugin.rc
prebag-rc src/openmedialib/plugins/rubberband/msw/rubberbandplugin.rc
prebag-rc src/openmedialib/plugins/sdl/msw/sdlplugin.rc
prebag-rc src/openmedialib/plugins/sox/msw/soxplugin.rc
prebag-rc src/openmedialib/plugins/wav/msw/wavplugin.rc
prebag-rc src/openmedialib/tools/amlbatch/amlbatch.rc
prebag-rc src/openpluginlib/pl/msw/pluginlib.rc


echo "Updating RC files done."

fi 
