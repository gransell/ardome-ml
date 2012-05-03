// ml - A media library representation.

// Copyright (C) 2011 Vizrt
// Released under the LGPL.

#ifndef ML_FIX_PACKET
#define ML_FIX_PACKET

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/packet.hpp>
#include <openmedialib/ml/types.hpp>

#include <opencorelib/cl/enforce_defines.hpp>

namespace olib { namespace openmedialib { namespace ml {

// Do any necessary modifications to the stream to make them decodable
ML_DECLSPEC void fix_stream( frame_type_ptr &frame );

} } }

#endif
