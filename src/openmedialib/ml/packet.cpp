// ml - A media library representation.

// Copyright (C) 2008 Ardendo
// Released under the LGPL.

#include "packet.hpp"
#include <map>
#include <boost/thread/recursive_mutex.hpp>

namespace il = olib::openimagelib::il;

namespace olib { namespace openmedialib { namespace ml {

typedef std::map< enum packet_id, packet_decoder_ptr > decoder_map;
typedef boost::recursive_mutex::scoped_lock scoped_lock;

static boost::recursive_mutex mutex;
static packet_decoder_ptr decoder;

void ML_DECLSPEC packet_handler( packet_decoder_ptr decode )
{
	scoped_lock lock( mutex );
	decoder = decode;
}

il::image_type_ptr ML_DECLSPEC packet_decode( packet_type_ptr packet )
{
	scoped_lock lock( mutex );
	return decoder ? decoder->decode( packet ) : il::image_type_ptr( );
}

} } }

