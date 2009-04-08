// ml - A media library representation.

// Copyright (C) 2008 Ardendo
// Released under the LGPL.

#include "packet.hpp"
#include "frame.hpp"
#include <map>
#include <boost/thread/recursive_mutex.hpp>

namespace il = olib::openimagelib::il;

namespace olib { namespace openmedialib { namespace ml {

typedef boost::recursive_mutex::scoped_lock scoped_lock;
static boost::recursive_mutex mutex;

typedef std::map< enum packet_id, packet_decoder_ptr > decoder_map_type;
typedef std::map< enum packet_id, packet_encoder_ptr > encoder_map_type;

static decoder_map_type decoder_map;
static encoder_map_type encoder_map;

/// Register a decode handler
void ML_DECLSPEC packet_handler( packet_decoder_ptr decode )
{
	scoped_lock lock( mutex );
	decoder_map[ decode->id( ) ] = decode;
}

/// Register an encode handler
void ML_DECLSPEC packet_handler( packet_encoder_ptr encode )
{
	scoped_lock lock( mutex );
	encoder_map[ encode->id( ) ] = encode;
}

/// Decode a packet
il::image_type_ptr ML_DECLSPEC packet_decode( packet_type_ptr packet )
{
	packet_decoder_ptr decoder;

	{
		scoped_lock lock( mutex );
		if ( decoder_map.find( packet->id( ) ) != decoder_map.end( ) )
			decoder = decoder_map[ packet->id( ) ];
	}

	return decoder ? decoder->decode( packet ) : il::image_type_ptr( );
}

// Decode a packet from a frame
il::image_type_ptr ML_DECLSPEC packet_decode( frame_type_ptr frame )
{
	if ( frame && frame->get_packet( ) )
		return packet_decode( frame->get_packet( ) );
	return il::image_type_ptr( );
}

// Encode a frame
packet_type_ptr ML_DECLSPEC packet_encode( frame_type_ptr frame, packet_id id )
{
	packet_encoder_ptr encoder;

	{
		scoped_lock lock( mutex );
		if ( encoder_map.find( id ) != encoder_map.end( ) )
			encoder = encoder_map[ id ];
	}

	return encoder ? encoder->encode( frame ) : packet_type_ptr( );
}

} } }

