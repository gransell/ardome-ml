// ml - A media library representation.

// Copyright (C) 2011 Vizrt
// Released under the LGPL.

#ifndef ML_FIX_PACKET
#define ML_FIX_PACKET

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/stream.hpp>
#include <openmedialib/ml/types.hpp>

#include <opencorelib/cl/enforce_defines.hpp>

namespace olib { namespace openmedialib { namespace ml {

/** 
	Do any necessary modifications to the stream to make them decodable
	@param frame The frame containing the stream that will be modified.
	@param overwrite If this set then the modifications to the stream will be made inplace
					 and any existsing headers etc will be overwritten.
*/
ML_DECLSPEC void fix_stream( frame_type_ptr &frame, bool overwrite = false );

} } }

#endif
