// ml - A media library representation.

// Copyright (C) 2008 Ardendo
// Released under the LGPL.

#include "packet.hpp"
#include <map>
#include <boost/thread/recursive_mutex.hpp>

namespace il = olib::openimagelib::il;

namespace olib { namespace openmedialib { namespace ml {

typedef boost::recursive_mutex::scoped_lock scoped_lock;
static boost::recursive_mutex mutex;
typedef std::map< enum packet_id, packet_decoder_ptr > decoder_map_type;
static decoder_map_type decoder_map;

void ML_DECLSPEC packet_handler( packet_decoder_ptr decode )
{
	scoped_lock lock( mutex );
	decoder_map[ decode->id( ) ] = decode;
}

il::image_type_ptr ML_DECLSPEC packet_decode( packet_type_ptr packet )
{
	scoped_lock lock( mutex );
	if ( decoder_map.find( packet->id( ) ) != decoder_map.end( ) )
		return decoder_map[ packet->id( ) ]->decode( packet );
	return il::image_type_ptr( );
}

} } }

